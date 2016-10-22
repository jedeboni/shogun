/*
 * Copyright (c) The Shogun Machine Learning Toolbox
 * Written (w) 2016 Heiko Strathmann
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the Shogun Development Team.
 */

#include <shogun/lib/config.h>
#include <shogun/lib/SGMatrix.h>
#include <shogun/lib/SGVector.h>
#include <shogun/distributions/kernel_exp_family/KernelExpFamily.h>
#include <shogun/io/SGIO.h>

#include "impl/Full.h"
#include "impl/kernel/Gaussian.h"

using namespace shogun;

CKernelExpFamily::CKernelExpFamily() : CSGObject()
{
	m_impl=NULL;
}

CKernelExpFamily::CKernelExpFamily(SGMatrix<float64_t> data,
			float64_t sigma, float64_t lambda, float memory_limit_gib) : CSGObject()
{
	REQUIRE(data.matrix, "Given observations cannot be empty.\n");
	REQUIRE(data.num_rows>0, "Dimension of given observations (%d) must be positive.\n", data.num_rows);
	REQUIRE(data.num_cols>0, "Number of given observations (%d) must be positive.\n", data.num_cols);
	REQUIRE(sigma>0, "Given sigma (%f) must be positive.\n", sigma);
	REQUIRE(lambda>0, "Given lambda (%f) must be positive.\n", lambda);

	auto kernel = new kernel_exp_family_impl::kernel::Gaussian(sigma);
	m_impl = new kernel_exp_family_impl::Full(data, kernel, lambda);

	auto N =  m_impl->get_num_lhs();
	auto D = m_impl->get_num_dimensions();
	auto ND = D*N;
	// size: A matrix, b vetor, all_hessians matrix, h vector, kernel matrix
	auto storage_size = (ND+1)*(ND+1) +(ND+1) +  ND*ND + ND + N*N;
	auto memory_required_gib = storage_size * sizeof(float64_t) / 8.0 / 1024.0 / 1024.0 / 1024.0;
	if (memory_required_gib > memory_limit_gib)
	{
		SG_ERROR("The problem's size (N=%d, D=%d) will at least use %f GiB of computer memory, "
				"which is above the set limit (%f GiB). "
				"In order to remove this error, increase this limit in the constructor.\n",
				N, D, memory_required_gib, memory_limit_gib);
	}
}

CKernelExpFamily::~CKernelExpFamily()
{
	delete m_impl;
	m_impl=NULL;
}

void CKernelExpFamily::fit()
{
	m_impl->fit();
}

float64_t CKernelExpFamily::log_pdf(index_t i)
{
	auto N = m_impl->get_num_rhs() ? m_impl->get_num_rhs() : m_impl->get_num_lhs();
	REQUIRE(i>=0 && i<N, "Given test data index (%d) must be in [0, %d].\n", i, N-1);
	return m_impl->log_pdf(i);
}

SGVector<float64_t> CKernelExpFamily::log_pdf_multiple()
{
	return m_impl->log_pdf();
}

SGMatrix<float64_t> CKernelExpFamily::grad_multiple()
{
	return m_impl->grad();
}

SGVector<float64_t> CKernelExpFamily::grad(index_t i)
{
	auto N = m_impl->get_num_rhs() ? m_impl->get_num_rhs() : m_impl->get_num_lhs();
	REQUIRE(i>=0 && i<N, "Given test data index (%d) must be in [0, %d].\n", i, N-1);
	return m_impl->grad(i);
}

SGMatrix<float64_t> CKernelExpFamily::hessian(index_t i)
{
	auto N = m_impl->get_num_rhs() ? m_impl->get_num_rhs() : m_impl->get_num_lhs();
	REQUIRE(i>=0 && i<N, "Given test data index (%d) must be in [0, %d].\n", i, N-1);
	return m_impl->hessian(i);
}

float64_t CKernelExpFamily::objective()
{
	return m_impl->objective();
}

SGVector<float64_t> CKernelExpFamily::get_alpha_beta()
{
	return m_impl->get_alpha_beta();
}

SGVector<float64_t> CKernelExpFamily::leverage()
{
	REQUIRE(m_impl->is_test_equals_train_data(),
			"Cannot proceed with test data. Reset test data!\n");
	return m_impl->leverage();
}

SGMatrix<float64_t> CKernelExpFamily::get_matrix(const char* name)
{
	REQUIRE(m_impl->is_test_equals_train_data(),
			"Cannot proceed with test data. Reset test data!\n");
	return m_impl->build_system().first;
}

SGVector<float64_t> CKernelExpFamily::get_vector(const char* name)
{
	REQUIRE(m_impl->is_test_equals_train_data(),
			"Cannot proceed with test data. Reset test data!\n");

	if (!strcmp(name, "alpha_beta"))
		return m_impl->get_alpha_beta();
	else if (!strcmp(name, "b"))
		return m_impl->build_system().second;
	else
		REQUIRE(false, "No vector with given name (%s).\n", name);

	return SGVector<float64_t>();
}

void CKernelExpFamily::reset_test_data()
{
	m_impl->reset_test_data();
}

void CKernelExpFamily::set_test_data(SGMatrix<float64_t> X)
{
	auto D = m_impl->get_num_dimensions();
	REQUIRE(X.matrix, "Given observations cannot be empty.\n");
	REQUIRE(X.num_rows==D, "Dimension of given observations (%d) must match the estimator's (%d).\n", X.num_rows, D);
	REQUIRE(X.num_cols>0, "Number of given observations (%d) must be positive.\n", X.num_cols);
	m_impl->set_test_data(X);
}

void CKernelExpFamily::set_test_data(SGVector<float64_t> x)
{
	auto D = m_impl->get_num_dimensions();
	REQUIRE(x.vector, "Given observations cannot be empty.\n");
	REQUIRE(x.vlen==D, "Dimension of given point (%d) must match the estimator's (%d).\n", x.vlen, D);
	m_impl->set_test_data(x);
}

