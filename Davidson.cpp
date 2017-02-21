#include "Davidson.h"
#include "Hmult.h"
#include <Eigen/Dense>
#include <iostream>
#include <iostream>
#ifndef SERIAL
#include <boost/mpi/environment.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/mpi.hpp>
#endif
#include "boost/format.hpp"
#include "communicate.h"
#include "iowrapper.h"
#include "global.h"

using namespace Eigen;
using namespace std;
using namespace boost;

std::complex<double> sumComplex(const std::complex<double>& a, const std::complex<double>& b) {
  return a+b;
};

void precondition(MatrixXx& r, MatrixXx& diag, double& e) {
  for (int i=0; i<r.rows(); i++) {
    if (abs(e-diag(i,0)) > 1e-12)
      r(i,0) = r(i,0)/(e-diag(i,0));
    else
      r(i,0) = r(i,0)/(e-diag(i,0)-1.e-8);
  }
}

//davidson, implemented very similarly to as implementeded in Block
//davidson, implemented very similarly to as implementeded in Block
vector<double> davidson(Hmult2& H, vector<MatrixXx>& x0, MatrixXx& diag, int maxCopies, double tol, bool print) {
#ifndef SERIAL
  boost::mpi::communicator world;
#endif
  std::vector<double> eroots;

  int nroots = x0.size();
  MatrixXx b=MatrixXx::Zero(x0[0].rows(), maxCopies); 

  int niter;
  //if some vector has zero norm then randomise it
  if (mpigetrank() == 0) {
    for (int i=0; i<nroots; i++)  {
      b.col(i) = 1.*x0[i];
      if (x0[i].norm() < 1.e-10) {
	b.col(i).setRandom();
	b.col(i) = b.col(i)/b.col(i).norm();
      }
    }
  }

  int size = b.rows()*maxCopies;
  MPI_Bcast(&(b(0,0)), size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  //mpi::broadcast(world, b, 0);

  //make vectors orthogonal to each other
  for (int i=0; i<x0.size(); i++) {  
    for (int j=0; j<i; j++) {
      CItype overlap = (b.col(j).adjoint()*b.col(i))(0,0);
      b.col(i) -= overlap*b.col(j);
    }
    if (b.col(i).norm() <1e-8) {
      b.col(i).setRandom();
    }
    for (int j=0; j<i; j++) {
      CItype overlap = (b.col(j).adjoint()*b.col(i))(0,0);
      b.col(i) -= overlap*b.col(j);
    }
    b.col(i) = b.col(i)/b.col(i).norm();
  }

  MatrixXx sigma = MatrixXx::Zero(x0[0].rows(), maxCopies); 
  int sigmaSize=0, bsize = x0.size();
  MatrixXx r(x0[0].rows(),1); r= 0.0*x0[0];
  int convergedRoot = 0;

  int iter = 0;

  while(true) {
    for (int i=sigmaSize; i<bsize; i++) {
      Eigen::Block<MatrixXx> bcol = b.block(0,i,b.rows(),1), sigmacol = sigma.block(0,i,sigma.rows(),1);
      H(bcol, sigmacol);
      sigmaSize++;
    }
    MatrixXx hsubspace(bsize, bsize);hsubspace.setZero(bsize, bsize);
    for (int i=0; i<bsize; i++)
      for (int j=i; j<bsize; j++) {
	hsubspace(i,j) = b.col(i).dot(sigma.col(j)); 
#ifdef Complex
	hsubspace(j,i) = conj(hsubspace(i,j));
#else
	hsubspace(j,i) = hsubspace(i,j);
#endif
      }

    SelfAdjointEigenSolver<MatrixXx> eigensolver(hsubspace);
    if (eigensolver.info() != Success) abort();

    b.block(0,0,b.rows(), bsize) = b.block(0,0,b.rows(), bsize)*eigensolver.eigenvectors();
    sigma.block(0,0,b.rows(), bsize) = sigma.block(0,0,b.rows(), bsize)*eigensolver.eigenvectors();

    double ei = eigensolver.eigenvalues()[convergedRoot];
    for (int i=0; i<convergedRoot; i++) {
      r = sigma.col(i) - eigensolver.eigenvalues()[i]*b.col(i);
      double error = r.norm();
      if (error > tol) {
	convergedRoot = i;
	pout << "going back to converged root "<<i<<endl;
	continue;
      }
    }

    r = sigma.col(convergedRoot) - ei*b.col(convergedRoot);
    double error = r.norm();

    if (iter == 0)
      pout << str(boost::format("#niter:%3d root:%3d -> Energy : %18.10g  \n") %(iter) % (convergedRoot-1) % ei );
    if (false)
      pout <<"#"<< iter<<" "<<convergedRoot<<"  "<<ei<<"  "<<error<<std::endl;
    iter++;

    if (hsubspace.rows() == b.rows()) {
      //all root are available
      for (int i=0; i<x0.size(); i++) {
	x0[i] = b.col(i);
	eroots.push_back(eigensolver.eigenvalues()[i]);
      }
      return eroots;
    }

    if (error < tol || iter >200) {
      if (iter >200) {
	cout << "Didnt converge"<<endl;
	exit(0);
	return eroots;
      }
      convergedRoot++;
      pout << str(boost::format("#niter:%3d root:%3d -> Energy : %18.10g  \n") %(iter) % (convergedRoot-1) % ei );
      if (convergedRoot == nroots) {
	for (int i=0; i<convergedRoot; i++) {
	  x0[i] = b.col(i);
	  eroots.push_back(eigensolver.eigenvalues()[i]);
	}
	return eroots;
      }
    }

    precondition(r,diag,ei);
    for (int i=0; i<bsize; i++) 
      r = r - (b.col(i).adjoint()*r)(0,0)*b.col(i);
    //r = r - (r.cwiseProduct(b.col(i)).sum())*b.col(i);
    //r = r - (b.col(i).transpose()*r)(0,0)*b.col(i);

    if (bsize < maxCopies) {
      b.col(bsize) = r/r.norm();
      bsize++;
    }
    else {
      bsize = nroots+3;
      sigmaSize = nroots+2;
      b.col(bsize-1) = r/r.norm();
    }
  }
}




double LinearSolver(Hmult2& H, MatrixXx& x0, MatrixXx& b, double tol, bool print) {
#ifndef SERIAL
    boost::mpi::communicator world;
#endif

  x0.setZero(x0.rows(),1);
  MatrixXx r = 1.*b, p = 1.*b;
  double rsold = r.squaredNorm();

  int iter = 0;
  while (true) {
    MatrixXx Ap = 0.*p; H(p,Ap);
    CItype alpha = rsold/(p.adjoint()*Ap)(0,0);
    x0 += alpha*p;
    r -= alpha*Ap;

    double rsnew = r.squaredNorm();
    CItype ept = -(x0.adjoint()*r + x0.adjoint()*b)(0,0);
    if (true)
      pout <<"#"<< iter<<" "<<ept<<"  "<<rsnew<<std::endl;
    if (r.norm() < tol || iter > 100) { 
      p.setZero(p.rows(),1); H(x0,p); p -=b; cout << (p.adjoint()*p)(0,0)<<endl; 
      return abs(ept);
    }      

    p = r +(rsnew/rsold)*p;
    rsold = rsnew;
    iter++;
  }

  
}






