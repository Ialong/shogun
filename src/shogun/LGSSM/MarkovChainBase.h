/*
 * Copyright (c) The Shogun Machine Learning Toolbox
 * Written (w) 2015 Alessandro Ialongo
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

#ifndef _MARKOVCHAINBASE_H__
#define _MARKOVCHAINBASE_H__

#include <shogun/lib/config.h>
#include <shogun/base/SGObject.h>

namespace shogun
{

/** @brief This is the base class for a Markov Chain (which is also instantiated as a latent chain in the LGSSM). 
*/
template <class T> class CMarkovChainBase : public CSGObject
{
	public:
		/** constructor */
		CMarkovChainBase() : CSGObject() { };

		/** destructor */
		virtual ~CMarkovChainBase() { };

		/** returns the name of the class */
		virtual const char* get_name() const { return "MarkovChainBase"; }

		/** generates a random Markov chain
		 *
		 * @return the values of the variables at each time point in a DxT matrix (D is the variable 
		 * dimensionality, T is the number of time samples)
		 */
		virtual SGMatrix<float64_t> generate_markov_chain()=0;

		/** Expectation Maximization algorithm - runs expectation step and maximization step repeatedly as long as
		 * max number of iterations is not reached or convergence does not take place.
		 *
		 * @param max_iters max number of iterations of EM
		 * @param epsilon convergence tolerance
		 * @return whether convergence is acheived
		 */
		bool iterate_em(int32_t max_iters=10000, float64_t epsilon=1e-8)
		{
			float64_t log_likelihood_cur=0;
			float64_t log_likelihood_prev=0;
			int32_t i=0;
			bool converge=false;
			while (i<max_iters)
			{
				log_likelihood_prev=log_likelihood_cur;
				log_likelihood_cur=expectation_step();

				if ((i>0)&&(log_likelihood_cur-log_likelihood_prev)<epsilon)
				{
					converge=true;
					break;
				}

				maximization_step();
				i++;
			}

			return converge;
		}

	public:
		/** data */
		T data;

};
} /* shogun */
#endif /* _MARKOVCHAINBASE_H__ */
