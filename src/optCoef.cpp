//Estimation of beta coefficients
#include "seqHMM.h"

unsigned int optCoef(arma::mat& weights, const arma::icube& obs, const arma::cube& emission,
    const arma::mat& initk, const arma::cube& beta, const arma::mat& scales, arma::mat& coef,
    const arma::mat& X, const arma::ivec& cumsumstate, const arma::ivec& numberOfStates,
    int trace) {

  int iter = 0;
  double change = 1.0;
  while ((change > 1e-10) & (iter < 100)) {
    arma::vec tmpvec(X.n_cols * (weights.n_rows - 1));
    bool solve_ok = arma::solve(tmpvec, hCoef(weights, X),
        gCoef(obs, beta, scales, emission, initk, weights, X, cumsumstate, numberOfStates));
    if (solve_ok == false) {
      return (4);
    }

    arma::mat coefnew(coef.n_rows, coef.n_cols - 1);
    for (unsigned int i = 0; i < (weights.n_rows - 1); i++) {
      coefnew.col(i) = coef.col(i + 1) - tmpvec.subvec(i * X.n_cols, (i + 1) * X.n_cols - 1);
    }
    change = arma::accu(arma::abs(coef.submat(0, 1, coef.n_rows - 1, coef.n_cols - 1) - coefnew))
        / coefnew.n_elem;
    coef.submat(0, 1, coef.n_rows - 1, coef.n_cols - 1) = coefnew;
    iter++;
    if (trace == 3) {
      Rcout << "coefficient optimization iter: " << iter;
      Rcout << " new coefficients: " << std::endl << coefnew << std::endl;
      Rcout << " relative change: " << change << std::endl;
    }
    weights = exp(X * coef).t();
    if (!weights.is_finite()) {
      return (5);
    }
    weights.each_row() /= sum(weights, 0);

  }
  return (0);
}

arma::vec gCoef(const arma::icube& obs, const arma::cube& beta, const arma::mat& scales,
    const arma::cube& emission, const arma::mat& initk, const arma::mat& weights,
    const arma::mat& X, const arma::ivec& cumsumstate, const arma::ivec& numberOfStates) {

  int q = X.n_cols;
  arma::vec grad(q * (weights.n_rows - 1), arma::fill::zeros);
  double tmp;

  for (unsigned int k = 0; k < obs.n_slices; k++) {
    for (int jj = 1; jj < numberOfStates.n_elem; jj++) {
      for (int j = 0; j < emission.n_rows; j++) {
        tmp = 1.0;
        for (unsigned int r = 0; r < obs.n_rows; r++) {
          tmp *= emission(j, obs(r, 0, k), r);
        }
        if ((j >= (cumsumstate(jj) - numberOfStates(jj))) & (j < cumsumstate(jj))) {
          grad.subvec(q * (jj - 1), q * jj - 1) += tmp * beta(j, 0, k) / scales(0, k) * initk(j, k)
              * X.row(k).t() * (1.0 - weights(jj, k));
        } else {
          grad.subvec(q * (jj - 1), q * jj - 1) -= tmp * beta(j, 0, k) / scales(0, k) * initk(j, k)
              * X.row(k).t() * weights(jj, k);
        }
      }
    }
  }

  return grad;
}

arma::mat hCoef(const arma::mat& weights, const arma::mat& X) {

  int p = X.n_cols;
  arma::mat hess(p * (weights.n_rows - 1), p * (weights.n_rows - 1), arma::fill::zeros);
  for (unsigned int i = 0; i < X.n_rows; i++) {
    arma::mat XX = X.row(i).t() * X.row(i);
    for (unsigned int j = 0; j < (weights.n_rows - 1); j++) {
      for (unsigned int k = 0; k < (weights.n_rows - 1); k++) {
        if (j != k) {
          hess.submat(j * p, k * p, (j + 1) * p - 1, (k + 1) * p - 1) += XX * weights(j + 1, i)
              * weights(k + 1, i);
        } else {
          hess.submat(j * p, j * p, (j + 1) * p - 1, (j + 1) * p - 1) -= XX * weights(j + 1, i)
              * (1.0 - weights(j + 1, i));

        }

      }
    }
  }
  return hess;
}
