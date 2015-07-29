/*
 * Copyright (c) The Shogun Machine Learning Toolbox
 * Written (w) 2015 Alessandro Ialongo
 * Written (W) 2013 Heiko Strathmann
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
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
 *
 */


// Do we need to initialise m_dimension or is it somehow done automatically?
// How should we store (i.e. under which type) m_cov_type?

#include <shogun/distributions/classical/GaussianDistributionNew.h>

#ifdef HAVE_EIGEN3

#include <shogun/base/Parameter.h>
#include <shogun/mathematics/eigen3.h>

using namespace shogun;
using namespace Eigen;

CGaussianDistributionNew::CGaussianDistributionNew() : CProbabilityDistribution()
{
	init();
}

CGaussianDistributionNew::CGaussianDistributionNew(SGVector<float64_t> mean,
		float64_t cov, bool cov_is_cholesky) :
				CProbabilityDistribution(mean.vlen)
{
	init();

	m_mean=mean;

	m_cov=SGMatrix<float64_t>(1,1);
	m_cov(0,0)=cov;

	m_cov_type="Float";
}

CGaussianDistributionNew::CGaussianDistributionNew(SGVector<float64_t> mean,
		SGVector<float64_t> cov, bool cov_is_cholesky) :
				CProbabilityDistribution(mean.vlen)
{
	REQUIRE(mean.vlen==cov.vlen,
			"Mean dimension (%d) and covariance dimension (%d) do not match\n",
			mean.vlen, cov.vlen);

	init();

	m_mean=mean;

	m_cov=cov.convert_to_matrix(cov,1,cov.vlen,false);

	m_cov_type="Vector";
}

CGaussianDistributionNew::CGaussianDistributionNew(SGVector<float64_t> mean,
		SGMatrix<float64_t> cov, bool cov_is_cholesky) :
				CProbabilityDistribution(mean.vlen)
{
	REQUIRE(cov.num_rows==cov.num_cols, "Covariance must be square but is "
			"%dx%d\n", cov.num_rows, cov.num_cols);
	REQUIRE(mean.vlen==cov.num_cols,
			"Mean dimension (%d) and covariance dimension (%d) do not match\n",
			mean.vlen, cov.num_cols);

	init();

	m_mean=mean;

	if (!cov_is_cholesky)
	{
		compute_cholesky(cov);
	}
	else
		m_cov=cov;

	m_cov_type="Matrix";
}

CGaussianDistributionNew::~CGaussianDistributionNew()
{

}

void CGaussianDistributionNew::set_mean(SGVector<float64_t> mean)
{
	m_mean=mean;
}

void CGaussianDistributionNew::set_cov(float64_t cov)
{
	m_cov=SGMatrix<float64_t>(1,1);
	m_cov(0,0)=cov;

	m_cov_type="Float";
}

void CGaussianDistributionNew::set_cov(SGVector<float64_t> cov)
{
	m_cov=cov.convert_to_matrix(cov,1,cov.vlen,false);

	m_cov_type="Vector";
}

void CGaussianDistributionNew::set_cov(SGMatrix<float64_t> cov)
{
	m_cov=cov;

	m_cov_type="Matrix";
}

SGVector<float64_t> CGaussianDistributionNew::get_mean() const
{
	return m_mean;
}

float64_t CGaussianDistributionNew::get_cov_spherical() const
{
	return *m_cov.data();
}

SGVector<float64_t> CGaussianDistributionNew::get_cov_diag() const
{
	return m_cov.get_row_vector(0);
}

SGMatrix<float64_t> CGaussianDistributionNew::get_cov_full() const
{
	return m_cov;
}

char* CGaussianDistributionNew::get_cov_type() const
{
	return m_cov_type;
}

SGMatrix<float64_t> CGaussianDistributionNew::sample(int32_t num_samples,
		SGMatrix<float64_t> pre_samples) const
{
	REQUIRE(num_samples>0, "Number of samples (%d) must be positive\n",
			num_samples);

	/* use pre-allocated samples? */
	SGMatrix<float64_t> samples;
	if (pre_samples.matrix)
	{
		REQUIRE(pre_samples.num_rows==m_dimension, "Dimension of pre-samples"
				" (%d) does not match dimension of Gaussian (%d)\n",
				pre_samples.num_rows, m_dimension);

		REQUIRE(pre_samples.num_cols==num_samples, "Number of pre-samples"
				" (%d) does not desired number of samples (%d)\n",
				pre_samples.num_cols, num_samples);

		samples=pre_samples;
	}
	else
	{
		/* allocate memory and sample from std normal */
		samples=SGMatrix<float64_t>(m_dimension, num_samples);
		for (index_t i=0; i<m_dimension*num_samples; ++i)
			samples.matrix[i]=sg_rand->std_normal_distrib();
	}

	/* map into desired Gaussian covariance */
	Map<MatrixXd> eigen_samples(samples.matrix, samples.num_rows,
			samples.num_cols);
	Map<MatrixXd> eigen_L(m_cov.matrix, m_cov.num_rows, m_cov.num_cols);
	eigen_samples=eigen_L*eigen_samples;

	/* add mean */
	Map<VectorXd> eigen_mean(m_mean.vector, m_mean.vlen);
	eigen_samples.colwise()+=eigen_mean;

	return samples;
}

void CGaussianDistributionNew::compute_cholesky(SGMatrix<float64_t> cov)
{
	Map<MatrixXd> eigen_cov(cov.matrix, cov.num_rows, cov.num_cols);
	m_cov=SGMatrix<float64_t>(cov.num_rows, cov.num_cols);
	Map<MatrixXd> eigen_L(m_cov.matrix, m_cov.num_rows, m_cov.num_cols);

	/* compute cholesky */
	LLT<MatrixXd> llt(eigen_cov);
	if (llt.info()==NumericalIssue)
	{
		/* try to compute smallest eigenvalue for information */
		SelfAdjointEigenSolver<MatrixXd> solver(eigen_cov);
		if (solver.info() == Success)
		{
			VectorXd ev=solver.eigenvalues();
			SG_ERROR("Error computing Cholesky of Gaussian's covariance. "
					"Smallest Eigenvalue is %f.\n", ev[0]);
		}
		else
			SG_ERROR("Error computing Cholesky of Gaussian's covariance. "
					"Numerical Issue (could not compute eigenvalues)");
	}
	eigen_L = llt.matrixL();
}

SGVector<float64_t> CGaussianDistributionNew::log_pdf_multiple(SGMatrix<float64_t> samples) const
{
	REQUIRE(samples.num_cols>0, "Number of samples must be positive, but is %d\n",
			samples.num_cols);
	REQUIRE(samples.num_rows==m_dimension, "Sample dimension (%d) does not match"
			"Gaussian dimension (%d)\n", samples.num_rows, m_dimension);

	/* for easier to read code */
	index_t num_samples=samples.num_cols;

	float64_t const_part=-0.5 * m_dimension * CMath::log(2 * CMath::PI);

	/* determinant is product of diagonal elements of triangular matrix */
	float64_t log_det_part=0;
	Map<MatrixXd> eigen_L(m_cov.matrix, m_cov.num_rows, m_cov.num_cols);
	VectorXd diag=eigen_L.diagonal();
	log_det_part=-diag.array().log().sum();

	/* sample based part */
	Map<MatrixXd> eigen_samples(samples.matrix, samples.num_rows,
			samples.num_cols);
	Map<VectorXd> eigen_mean(m_mean.vector, m_mean.vlen);

	/* substract mean from samples (create copy) */
	SGMatrix<float64_t> centred(m_dimension, num_samples);
	Map<MatrixXd> eigen_centred(centred.matrix, centred.num_rows,
			centred.num_cols);
	for (index_t dim=0; dim<m_dimension; ++dim)
	{
		for (index_t sample_idx=0; sample_idx<num_samples; ++sample_idx)
			centred(dim,sample_idx)=samples(dim,sample_idx)-m_mean[dim];
	}

	/* solve the linear system based on factorization */
	MatrixXd solved=eigen_L.triangularView<Lower>().solve(eigen_centred);
	solved=eigen_L.transpose().triangularView<Upper>().solve(solved);

	/* one quadratic part x^T C^-1 x for each sample x */
	SGVector<float64_t> result(num_samples);
	Map<VectorXd> eigen_result(result.vector, result.vlen);
	for (index_t i=0; i<num_samples; ++i)
	{
		/* i-th centred sample */
		VectorXd left=eigen_centred.block(0, i, m_dimension, 1);

		/* inverted covariance times i-th centred sample */
		VectorXd right=solved.block(0,i,m_dimension,1);
		result[i]=-0.5*left.dot(right);
	}

	/* combine and return */
	eigen_result=eigen_result.array()+(log_det_part+const_part);

	/* contains everything */
	return result;

}

void CGaussianDistributionNew::init()
{
	SG_ADD(&m_mean, "mean", "Mean of the Gaussian.", MS_NOT_AVAILABLE);
	SG_ADD(&m_cov, "cov", "Covariance matrix, "
			"can be float, SGVector, or SGMatrix (cholesky).", MS_NOT_AVAILABLE);
//	SG_ADD(&m_cov_type, "cov_type", "Type of covariance matrix, "
//				"can be 'Float', 'Vector', or 'Matrix'.", MS_NOT_AVAILABLE);
}

#endif // HAVE_EIGEN3
