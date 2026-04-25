/***************************************************************************
* Copyright (C) 2011-2016, Crystice Softworks.
* 
* This file is part of QindieGL source code.
* Please note that QindieGL is not driver, it's emulator.
* 
* QindieGL source code is free software; you can redistribute it and/or 
* modify it under the terms of the GNU General Public License as 
* published by the Free Software Foundation; either version 2 of 
* the License, or (at your option) any later version.
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
#include "d3d_wrapper.hpp"
#include "d3d_matrix_stack.hpp"

D3DStateMatrix :: D3DStateMatrix()
{
	set_dirty();
}

void D3DStateMatrix :: set_dirty()
{
	m_inverse_dirty = TRUE;
	m_transpose_dirty = TRUE;
	m_invtrans_dirty = TRUE;
	m_identity = FALSE;
}

void D3DStateMatrix :: set_identity()
{
	m_matrix = DirectX::XMMatrixIdentity();
	m_inverse_dirty = TRUE;
	m_transpose_dirty = TRUE;
	m_invtrans_dirty = TRUE;
	m_identity = TRUE;
}

void D3DStateMatrix :: check_inverse()
{
	if (m_inverse_dirty) {
		if (m_identity) {
			m_inverse = m_matrix;
		} else {
			m_inverse = DirectX::XMMatrixInverse( nullptr, m_matrix );
		}
		m_inverse_dirty = FALSE;
	}
}

void D3DStateMatrix :: check_transpose()
{
	if (m_transpose_dirty) {
		if (m_identity) {
			m_transpose = m_matrix;
		} else {
			m_transpose = DirectX::XMMatrixTranspose(m_matrix);
		}
		m_transpose_dirty = FALSE;
	}
}

void D3DStateMatrix :: check_invtrans()
{
	if (m_invtrans_dirty) {
		if (m_identity) {
			m_invtrans = m_matrix;
		} else {
			check_inverse();
			m_invtrans = XMMatrixTranspose(m_inverse);
		}
		m_invtrans_dirty = FALSE;
	}
}

//----------------------------------------------------------------

D3DMatrixStack :: D3DMatrixStack() : m_iStackDepth( 0 )
{
	for ( int i = 0; i < D3D_MAX_MATRIX_STACK_DEPTH; ++i) {
		m_Stack[i].set_identity();
	}
}

void D3DMatrixStack :: load_identity()
{
	this->top().set_identity();
}

void D3DMatrixStack :: load(DirectX::XMMATRIX m )
{
	this->top() = m;
}

void D3DMatrixStack::multiply(DirectX::XMMATRIX m)
{
	this->top() = DirectX::XMMatrixMultiply(m, this->top());
}

HRESULT D3DMatrixStack :: push()
{
	if (m_iStackDepth == (D3D_MAX_MATRIX_STACK_DEPTH-1))
		return E_STACK_OVERFLOW;
	++m_iStackDepth;
	this->m_Stack[m_iStackDepth] = this->m_Stack[m_iStackDepth - 1];
	return S_OK;
}

HRESULT D3DMatrixStack :: pop()
{
	if (!m_iStackDepth)
		return E_STACK_UNDERFLOW;
	--m_iStackDepth;
	return S_OK;
}

D3DStateMatrix::operator D3DMATRIX() const
{
	DirectX::XMFLOAT4X4A temp;
	DirectX::XMStoreFloat4x4A(&temp, m_matrix);
	D3DMATRIX m;
	memcpy(&m, temp.m, sizeof(D3DMATRIX));
	return m;
}