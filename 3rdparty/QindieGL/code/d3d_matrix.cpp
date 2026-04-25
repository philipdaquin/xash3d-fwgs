/***************************************************************************
* Copyright( C ) 2011-2016, Crystice Softworks.
* 
* This file is part of QindieGL source code.
* Please note that QindieGL is not driver, it's emulator.
* 
* QindieGL source code is free software; you can redistribute it and/or 
* modify it under the terms of the GNU General Public License as 
* published by the Free Software Foundation; either version 2 of 
* the License, or( at your option ) any later version.
* 
* QindieGL source code is distributed in the hope that it will be 
* useful, but WITHOUT ANY WARRANTY; without even the implied 
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
* See the GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software 
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
***************************************************************************/
#include <algorithm>

#include "d3d_wrapper.hpp"
#include "d3d_global.hpp"
#include "d3d_state.hpp"
#include "d3d_matrix_stack.hpp"

//==================================================================================
// Some words about projection matrices
//----------------------------------------------------------------------------------
// They are different in OpenGL and Direct3D because of different clip space.
// glFrustum will create a D3D-compatible projection matrix.
// But some games( like Quake3 and Doom3 ) create their own projection matrices 
// and glLoadMatrix them. If ProjectionFix setting is enabled, we will 
// try to catch such operation and fix the matrix.
//
// The fix is applied to values C( m[2][2] ) and D( m[2][3] ).
//
// In OpenGL, perspective projection matrix is defined in this way:
// C =( zn + zf ) /( zn - zf )
// D = 2 * zn * zf /( zn - zf )
// In Direct3D they are a bit different:
// C = zf /( zn - zf )
// D = zn * zf /( zn - zf )
// So, we restore zn and zf from OpenGL matrix, and then subtract zn/( zn - zf ) from C 
// and scale D by one half.
//
// In OpenGL, ortho projection matrix is defined in this way:
// C = 2 /( zn - zf )
// D =( zn + zf ) /( zn - zf )
// In Direct3D they are a bit different:
// C = 1 /( zn - zf )
// D = zn /( zn - zf )
// So, we restore zn and zf from OpenGL matrix, and then subtract zf/( zn - zf ) from D 
// and scale C by one half.
//
// Without this fix there will be noticeable clipping problems on viewmodels in games that
// don't use glFrustum and load projection matrices themselves. Also, Doom3 won't draw any
// 2D graphics.
//
//==================================================================================
// Matrix operation functions
//----------------------------------------------------------------------------------
// We use some D3DX functionality to implement it
//==================================================================================

static inline void CheckTexCoordOffset_Hack( bool ortho )
{
	if( !D3DGlobal.settings.texcoordFix )
		return;

	//HACK: OpenGL somehow offsets texcoords in 2D mode??!
	//HACK: so we will create a workaround for it
	//HACK: without this fix there will be no crosshair in Quake2
	//HACK: also there will be a problem with sprites and fonts in Xash
	if( D3DState.TransformState.matrixMode == GL_PROJECTION ) {
		if( ortho ) {
			D3DState.TransformState.texcoordFixEnabled = TRUE;
		} else {
			D3DState.TransformState.texcoordFixEnabled = FALSE;
		}
	}
}

OPENGL_API void glMatrixMode( GLenum mode )
{
	if( D3DState.TransformState.matrixMode != mode )
	{
		D3DState.TransformState.matrixMode = mode;
		if( !D3DState_SetMatrixMode() ) 
			logPrintf( "WARNING: glMatrixMode: unimplemented matrix mode 0x%x\n", mode );
	}
}

OPENGL_API void glLoadIdentity()
{
	if( !D3DState.currentMatrixStack ) return;
	D3DState.currentMatrixStack->load_identity( );
	*D3DState.currentMatrixModified = true;
	CheckTexCoordOffset_Hack( false );
}

static void ProjectionMatrix_GLtoD3D(DirectX::XMFLOAT4X4 &m )
{
	if( m(2, 3) >= 0 ) {
		//2D projection
		//Restore znear and zfar from projection matrix
		GLfloat fC = m(2, 2);
		GLfloat fD = m(3, 2);
		GLfloat fQ =( fD + 1.0f ) /( fD - 1.0f );
		GLfloat zF = 2.0f /( fC *( fQ - 1.0f ) );
		GLfloat zN =( 2.0f / fC ) + zF;
		//Convert GL ortho projection to D3D
		m(2, 2) *= 0.5f;
		m(3, 2) -= zF /( zN - zF );
	} else {
		//3D projection
		//Restore znear and zfar from projection matrix
		GLfloat fC = m(2, 2);
		GLfloat fD = m(3, 2);
		GLfloat fQ =( 1.0f + fC ) /( 1.0f - fC );
		GLfloat zF =( fD *( 1.0f + fQ ) ) /( 2.0f * fQ );
		GLfloat zN =( fD * zF ) /( fD - 2.0f*zF );
		//Convert GL perspective projection to D3D
		m(2, 2) -= zN /( zN - zF );
		m(3, 2) *= 0.5f;
	}
}

OPENGL_API void glLoadMatrixf( const GLfloat *m )
{
	if( !D3DState.currentMatrixStack ) return;
	bool b2Dproj = false;
	DirectX::XMMATRIX mt(m);
	if( D3DGlobal.settings.projectionFix ) {
		if( D3DState.TransformState.matrixMode == GL_PROJECTION ) {
			DirectX::XMFLOAT4X4A mt4;
			DirectX::XMStoreFloat4x4A(&mt4, mt);
			b2Dproj =(mt4(2, 3) >= 0 );
			ProjectionMatrix_GLtoD3D(mt4);
			mt = DirectX::XMLoadFloat4x4A(&mt4);
		}
		D3DState.currentMatrixStack->load(mt);
	} else {
		D3DState.currentMatrixStack->load(mt);
	}
	*D3DState.currentMatrixModified = true;
	CheckTexCoordOffset_Hack( b2Dproj );
}
OPENGL_API void glLoadMatrixd( const GLdouble *m )
{
	FLOAT mf[16];
	std::copy_n(m, 16, mf);
	return glLoadMatrixf(mf);
}
OPENGL_API void glMultMatrixf( const GLfloat *m )
{
	if( !D3DState.currentMatrixStack ) return;
	D3DState.currentMatrixStack->multiply(DirectX::XMMATRIX(m));
	*D3DState.currentMatrixModified = true;
	CheckTexCoordOffset_Hack( false );
}
OPENGL_API void glMultMatrixd( const GLdouble *m )
{
	FLOAT mf[16];
	std::copy_n(m, 16, mf);
	return glMultMatrixf(mf);
}
OPENGL_API void glLoadTransposeMatrixf( const GLfloat *m )
{
	if( !D3DState.currentMatrixStack ) return;
	bool b2Dproj = false;
	DirectX::XMMATRIX mt = DirectX::XMMatrixTranspose(DirectX::XMMATRIX(m));
	
	if( D3DGlobal.settings.projectionFix ) {
		if( D3DState.TransformState.matrixMode == GL_PROJECTION ) {
			DirectX::XMFLOAT4X4A mt4;
			DirectX::XMStoreFloat4x4A(&mt4, mt);
			b2Dproj =( mt4(2,3) >= 0 );
			ProjectionMatrix_GLtoD3D(mt4);
			mt = DirectX::XMLoadFloat4x4A(&mt4);
		}
	}
	D3DState.currentMatrixStack->load( mt );
	*D3DState.currentMatrixModified = true;
	CheckTexCoordOffset_Hack( b2Dproj );
}
OPENGL_API void glLoadTransposeMatrixd( const GLdouble *m )
{
	FLOAT mf[16];
	std::copy_n(m, 16, mf);
	return glLoadTransposeMatrixf(mf);
}
OPENGL_API void glMultTransposeMatrixf( const GLfloat *m )
{
	if( !D3DState.currentMatrixStack ) return;
	DirectX::XMMATRIX mt = DirectX::XMMatrixTranspose(DirectX::XMMATRIX(m));
	
	D3DState.currentMatrixStack->multiply( mt );
	*D3DState.currentMatrixModified = true;
	CheckTexCoordOffset_Hack( false );
}
OPENGL_API void glMultTransposeMatrixd( const GLdouble *m )
{
	FLOAT mf[16];
	std::copy_n(m, 16, mf);
	return glMultTransposeMatrixf(mf);
}
OPENGL_API void glFrustum( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar )
{
	if( !D3DState.currentMatrixStack ) return;
	DirectX::XMMATRIX m = DirectX::XMMatrixPerspectiveOffCenterRH((FLOAT)left, (FLOAT)right, (FLOAT)bottom, (FLOAT)top, (FLOAT)zNear, (FLOAT)zFar);
	D3DState.currentMatrixStack->multiply( m );
	*D3DState.currentMatrixModified = true;
	CheckTexCoordOffset_Hack( false );
}
OPENGL_API void glOrtho( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar )
{
	if( !D3DState.currentMatrixStack ) return;
	DirectX::XMMATRIX m = DirectX::XMMatrixOrthographicOffCenterRH((FLOAT)left, (FLOAT)right, (FLOAT)bottom, (FLOAT)top, (FLOAT)zNear, (FLOAT)zFar);
	D3DState.currentMatrixStack->multiply( m );
	*D3DState.currentMatrixModified = true;
	CheckTexCoordOffset_Hack( true );
}
OPENGL_API void glPopMatrix( void )
{
	if( !D3DState.currentMatrixStack ) return;
	HRESULT hr = D3DState.currentMatrixStack->pop( );
	if( FAILED( hr ) ) D3DGlobal.lastError = hr;
	*D3DState.currentMatrixModified = true;
}
OPENGL_API void glPushMatrix( void )
{
	if( !D3DState.currentMatrixStack ) return;
	HRESULT hr = D3DState.currentMatrixStack->push( );
	if( FAILED( hr ) ) D3DGlobal.lastError = hr;
}
OPENGL_API void glRotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z )
{
	if( !D3DState.currentMatrixStack ) return;
	DirectX::XMMATRIX m = DirectX::XMMatrixRotationAxis(DirectX::XMVectorSet(x, y, z, 0), DirectX::XMConvertToRadians(angle));
	D3DState.currentMatrixStack->multiply( m );
	*D3DState.currentMatrixModified = true;
}
OPENGL_API void glRotated( GLdouble angle, GLdouble x, GLdouble y, GLdouble z )
{
	return glRotatef((FLOAT)angle, (FLOAT)x, (FLOAT)y, (FLOAT)z);
}
OPENGL_API void glScalef( GLfloat x, GLfloat y, GLfloat z )
{
	if( !D3DState.currentMatrixStack ) return;
	DirectX::XMMATRIX m = DirectX::XMMatrixScaling(x, y, z);
	D3DState.currentMatrixStack->multiply( m );
	*D3DState.currentMatrixModified = true;
}
OPENGL_API void glScaled( GLdouble x, GLdouble y, GLdouble z )
{
	return glScalef((FLOAT)x, (FLOAT)y, (FLOAT)z);
}
OPENGL_API void glTranslatef( GLfloat x, GLfloat y, GLfloat z )
{
	if( !D3DState.currentMatrixStack ) return;
	DirectX::XMMATRIX m = DirectX::XMMatrixTranslation(x, y, z);
	D3DState.currentMatrixStack->multiply( m );
	*D3DState.currentMatrixModified = true;
}
OPENGL_API void glTranslated( GLdouble x, GLdouble y, GLdouble z )
{
	return glTranslatef((FLOAT)x, (FLOAT)y, (FLOAT)z);
}