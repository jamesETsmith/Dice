#include "Determinants.h"
#include "SHCIbasics.h"
#include "SHCIgetdeterminants.h"
#include "SHCIsampledeterminants.h"
#include "input.h"
#include "integral.h"
#include <vector>
#include "math.h"
#include "Hmult.h"
#include <tuple>
#include <map>
#include "Davidson.h"
#include "boost/format.hpp"
#include <fstream>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/set.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#ifndef SERIAL
#include <boost/mpi/environment.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/mpi.hpp>
#endif
#include "communicate.h"

using namespace std;
using namespace Eigen;
using namespace boost;

template <class T> void reorder(vector<T>& A, std::vector<long>& reorder)
{
  vector<T> Acopy = A;
  for (int i=0; i<Acopy.size(); i++) {
    A[i] = Acopy[reorder[i]];
  }
}

void RemoveDuplicates(std::vector<Determinant>& Det,
		      std::vector<double>& Num1, std::vector<double>& Num2,
		      std::vector<double>& Energy, std::vector<char>& present) {

  size_t uniqueSize = 0;
  for (size_t i=1; i <Det.size(); i++) {
    if (!(Det[i] == Det[i-1])) {
      uniqueSize++;
      Det.at(uniqueSize) = Det[i];
      Num1.at(uniqueSize) = Num1[i];
      Num2.at(uniqueSize) = Num2[i];
      Energy.at(uniqueSize) = Energy[i];
      present.at(uniqueSize) = present[i];
    }
    else {
      Num1.at(uniqueSize) += Num1[i];
      Num2.at(uniqueSize) += Num1[i];
    }
  }
  Det.resize(uniqueSize+1);
  Num1.resize(uniqueSize+1);
  Num2.resize(uniqueSize+1);
  Energy.resize(uniqueSize+1);
  present.resize(uniqueSize+1);
}

void RemoveDetsPresentIn(std::vector<Determinant>& SortedDets, std::vector<Determinant>& Det,
			 std::vector<double>& Num1, std::vector<double>& Num2,
			 std::vector<double>& Energy, std::vector<char>& present) {

  vector<Determinant>::iterator vec_it = SortedDets.begin();
  vector<Determinant>::iterator vec_end = SortedDets.end();

  size_t uniqueSize = 0;
  for (size_t i=0; i<Det.size();) {
    if (Det[i] < *vec_it) {
      Det[uniqueSize] = Det[i];
      Num1[uniqueSize] = Num1[i];
      Num2[uniqueSize] = Num2[i];
      Energy[uniqueSize] = Energy[i];
      present[uniqueSize] = present[i];
      i++; uniqueSize++;
    }
    else if (*vec_it < Det[i] && vec_it != vec_end)
      vec_it ++;
    else if (*vec_it < Det[i] && vec_it == vec_end) {
      Det[uniqueSize] = Det[i];
      Num1[uniqueSize] = Num1[i];
      Num2[uniqueSize] = Num2[i];
      Energy[uniqueSize] = Energy[i];
      present[uniqueSize] = present[i];
      i++; uniqueSize++;
    }
    else {
      i++;
    }
  }
  Det.resize(uniqueSize); Num1.resize(uniqueSize);
  Num2.resize(uniqueSize); Energy.resize(uniqueSize);
  present.resize(uniqueSize);
}

void RemoveDuplicates(vector<Determinant>& Det) {
  if (Det.size() <= 1) return;
  std::vector<Determinant>& Detcopy = Det;
  size_t uniqueSize = 0;
  for (size_t i=1; i <Detcopy.size(); i++) {
    if (!(Detcopy[i] == Detcopy[i-1])) {
      uniqueSize++;
      Det[uniqueSize] = Detcopy[i];
    }
  }
  Det.resize(uniqueSize+1);
}

int partition(Determinant* A, int p,int q, CItype* pNum, double* Energy, vector<double>* det_energy=NULL, vector<char>* present=NULL)
{
  Determinant x= A[p];
  int i=p;
  int j;

  for(j=p+1; j<q; j++)
    {
      if(A[j]<x || A[j] == x)
        {
	  i=i+1;
	  swap(A[i],A[j]);
	  swap(pNum[i], pNum[j]);
	  swap(Energy[i], Energy[j]);

	  if (present != NULL)
	    swap(present->at(i), present->at(j));
	  if (det_energy != NULL)
	    swap(det_energy->at(i), det_energy->at(j));
        }

    }

  swap(A[i],A[p]);
  swap(pNum[i], pNum[p]);
  swap(Energy[i], Energy[p]);

  if (present != NULL)
    swap(present->at(i), present->at(p));
  if (det_energy != NULL)
    swap(det_energy->at(i), det_energy->at(p));
  return i;
}

int partitionAll(Determinant* A, int p,int q, CItype* pNum, double* Energy, vector<int>* var_indices, vector<size_t>* orbDifference, vector<double>* det_energy=NULL, vector<bool>* present=NULL)
{
  Determinant x= A[p];
  int i=p;
  int j;

  for(j=p+1; j<q; j++)
    {
      if(A[j]<x || A[j] == x)
        {
	  i=i+1;
	  swap(A[i],A[j]);
	  swap(pNum[i], pNum[j]);
	  swap(Energy[i], Energy[j]);
	  swap(var_indices[i], var_indices[j]);
	  swap(orbDifference[i], orbDifference[j]);

	  if (present != NULL) {
	    bool bkp = present->operator[](j);
	    present->operator[](j) = present->operator[](i);
	    present->operator[](i) = bkp;
	  }
	  if (det_energy != NULL)
	    swap(det_energy->operator[](i), det_energy->operator[](j));
        }

    }

  swap(A[i],A[p]);
  swap(pNum[i], pNum[p]);
  swap(Energy[i], Energy[p]);
  swap(var_indices[i], var_indices[p]);
  swap(orbDifference[i], orbDifference[p]);
  if (present != NULL) {
    bool bkp = present->operator[](p);
    present->operator[](p) = present->operator[](i);
    present->operator[](i) = bkp;
  }

  if (det_energy != NULL)
    swap(det_energy->operator[](i), det_energy->operator[](p));
  return i;
}

void quickSort(Determinant* A, int p,int q, CItype* pNum, double* Energy, vector<double>* det_energy=NULL, vector<char>* present=NULL)
{
  int r;
  if(p<q)
    {
      r=partition(A,p,q, pNum, Energy, det_energy, present);
      quickSort(A,p,r, pNum, Energy, det_energy, present);
      quickSort(A,r+1,q, pNum, Energy, det_energy, present);
    }
}

void quickSortAll(Determinant* A, int p,int q, CItype* pNum, double* Energy, vector<int>* var_indices, vector<size_t>* orbDifference, vector<double>* det_energy=NULL, vector<bool>* present=NULL)
{
  int r;
  if(p<q)
    {
      r=partitionAll(A,p,q, pNum, Energy, var_indices, orbDifference, det_energy, present);
      quickSortAll(A,p,r, pNum, Energy, var_indices, orbDifference, det_energy, present);
      quickSortAll(A,r+1,q, pNum, Energy, var_indices, orbDifference, det_energy, present);
    }
}



void merge(Determinant *a, long low, long high, long mid, long* x, Determinant* c, long* cx)
{
  long i, j, k;
  i = low;
  k = low;
  j = mid + 1;
  while (i <= mid && j <= high)
    {
      if (a[i] < a[j])
        {
	  c[k] = a[i];
	  cx[k] = x[i];
	  k++;
	  i++;
        }
      else
        {
	  c[k] = a[j];
	  cx[k] = x[j];
	  k++;
	  j++;
        }
    }
  while (i <= mid)
    {
      c[k] = a[i];
      cx[k] = x[i];
      k++;
      i++;
    }
  while (j <= high)
    {
      c[k] = a[j];
      cx[k] = x[j];
      k++;
      j++;
    }
  for (i = low; i < k; i++)
    {
      a[i] =  c[i];
      x[i] = cx[i];
    }
}

void mergesort(Determinant *a, long low, long high, long* x, Determinant* c, long* cx)
{
  long mid;
  if (low < high)
    {
      mid=(low+high)/2;
      mergesort(a,low,mid, x, c, cx);
      mergesort(a,mid+1,high, x, c, cx);
      merge(a,low,high,mid, x, c, cx);
    }
  return;
}


int ipow(int base, int exp)
{
  int result = 1;
  while (exp)
    {
      if (exp & 1)
	result *= base;
      exp >>= 1;
      base *= base;
    }

  return result;
}


class StitchDEH {
private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version) {
    ar & Det & Num & Num2 & Energy & var_indices & orbDifference;
  }

public:
  boost::shared_ptr<vector<Determinant> > Det;
  boost::shared_ptr<vector<CItype> > Num;       //the numerator for the first Var state
  boost::shared_ptr<vector<CItype> > Num2;      //the numberator for the second Var state (if present)
  boost::shared_ptr<vector<double> > Energy;

  //the next two store information about where a determinant in "C" arose from
  boost::shared_ptr<vector<int > > var_indices_beforeMerge;
  boost::shared_ptr<vector<size_t > > orbDifference_beforeMerge;

  //the next two store information about where a determinant in "C" arose from,
  //and takes into account the fact that a given determinant might have arisen from
  //multiple parent determinants in "V"
  boost::shared_ptr<vector<vector<int> > > var_indices;
  boost::shared_ptr<vector<vector<size_t> > > orbDifference;

  bool extra_info; // whether to use var_indices, orbDifference

  StitchDEH() {
    Det = boost::shared_ptr<vector<Determinant> > (new vector<Determinant>() );
    Num = boost::shared_ptr<vector<CItype> > (new vector<CItype>() );
    Num2 = boost::shared_ptr<vector<CItype> > (new vector<CItype>() );
    Energy = boost::shared_ptr<vector<double> > (new vector<double>() );
    extra_info = false;
    var_indices = boost::shared_ptr<vector<vector<int> > > (new vector<vector<int> >() );
    orbDifference = boost::shared_ptr<vector<vector<size_t> > > (new vector<vector<size_t> >() );
    var_indices_beforeMerge = boost::shared_ptr<vector<int > > (new vector<int >() );
    orbDifference_beforeMerge = boost::shared_ptr<vector<size_t > > (new vector<size_t >() );
  }

  StitchDEH(boost::shared_ptr<vector<Determinant> >pD,
	    boost::shared_ptr<vector<CItype> >pNum,
	    boost::shared_ptr<vector<CItype> >pNum2,
	    boost::shared_ptr<vector<double> >pE,
	    boost::shared_ptr<vector<vector<int> > >pvar,
	    boost::shared_ptr<vector<vector<size_t> > >porb,
	    boost::shared_ptr<vector<int > >pvar_beforeMerge,
	    boost::shared_ptr<vector<size_t > >porb_beforeMerge)
    : Det(pD), Num(pNum), Num2(pNum2), Energy(pE), var_indices(pvar), orbDifference(porb), var_indices_beforeMerge(pvar_beforeMerge), orbDifference_beforeMerge(porb_beforeMerge)
  {
    extra_info=true;
  };

  StitchDEH(boost::shared_ptr<vector<Determinant> >pD,
	    boost::shared_ptr<vector<CItype> >pNum,
	    boost::shared_ptr<vector<CItype> >pNum2,
	    boost::shared_ptr<vector<double> >pE)
    : Det(pD), Num(pNum), Num2(pNum2), Energy(pE)
  {
    extra_info=false;
  };

  void QuickSortAndRemoveDuplicates() {
    if (Num2->size() > 0) {
      cout << "Use mergesort instead"<<endl;
      exit(0);
    }

    if (extra_info) {
      quickSortAll(&(Det->operator[](0)), 0, Det->size(), &(Num->operator[](0)), &(Energy->operator[](0)), &(var_indices->operator[](0)), &(orbDifference->operator[](0)));
    } else {
      quickSort(&(Det->operator[](0)), 0, Det->size(), &(Num->operator[](0)), &(Energy->operator[](0)));
    }

    //if (Det->size() == 1) return;
    if (Det->size() <= 1) return;

    std::vector<Determinant>& Detcopy = *Det;
    std::vector<CItype>& Numcopy = *Num;
    std::vector<double>& Ecopy = *Energy;
    //if (extra_info) {
    std::vector<std::vector<int> >& Vcopy = *var_indices;
    std::vector<std::vector<size_t> >& Ocopy = *orbDifference;
    //}
    size_t uniqueSize = 0;
    for (size_t i=1; i <Detcopy.size(); i++) {
      if (!(Detcopy[i] == Detcopy[i-1])) {
	uniqueSize++;
	Det->operator[](uniqueSize) = Detcopy[i];
	Num->operator[](uniqueSize) = Numcopy[i];
	Energy->operator[](uniqueSize) = Ecopy[i];
        if (extra_info) {
	  var_indices->operator[](uniqueSize) = Vcopy[i];
	  orbDifference->operator[](uniqueSize) = Ocopy[i];
        }
      }
      else { // Same det, so combine
	Num->operator[](uniqueSize) += Numcopy[i];
        if (extra_info) {
          for (size_t k=0; k<(*var_indices)[i].size(); k++) {
    	    (*var_indices)[uniqueSize].push_back((*var_indices)[i][k]);
    	    (*orbDifference)[uniqueSize].push_back((*orbDifference)[i][k]);
          }
        }
      }
    }
    Det->resize(uniqueSize+1);
    Num->resize(uniqueSize+1);
    Energy->resize(uniqueSize+1);
    if (extra_info) {
      var_indices->resize(uniqueSize+1);
      orbDifference->resize(uniqueSize+1);
    }
  }

  void MergeSortAndRemoveDuplicates() {
    std::vector<Determinant> Detcopy = *Det;

    std::vector<long> detIndex(Detcopy.size(),0);
    std::vector<long> detIndexcopy(Detcopy.size(),0);

    for (size_t i=0; i<Detcopy.size(); i++)
      detIndex[i] = i;
    mergesort(&Detcopy[0], 0, Detcopy.size()-1, &detIndex[0], &( Det->operator[](0)), &detIndexcopy[0]);
    detIndexcopy.clear();

    bool varIndicesEmtpy = var_indices->size() == 0 ? true : false;

    if (Det->size() <= 1) return;
    reorder(*Num, detIndex);
    if (Num2->size() != 0)
      reorder(*Num2, detIndex);
    reorder(*Energy, detIndex);
    if (extra_info) {
      if (varIndicesEmtpy) {
	reorder(*var_indices_beforeMerge, detIndex);
	reorder(*orbDifference_beforeMerge, detIndex);
      }
      else {
	vector<vector<int> > Vcopy = *var_indices;
	for (int i=0; i<Vcopy.size(); i++) {
	  var_indices->at(i).clear();
	  var_indices->at(i).insert(var_indices->at(i).end(), Vcopy[detIndex[i]].begin(), Vcopy[detIndex[i]].end());
	}
	Vcopy.clear();
	vector<vector<size_t> > Ocopy = *orbDifference;
	for (int i=0; i<Ocopy.size(); i++) {
	  orbDifference->at(i).clear();
	  orbDifference->at(i).insert(orbDifference->at(i).end(), Ocopy[detIndex[i]].begin(), Ocopy[detIndex[i]].end());
	}
	Ocopy.clear();
      }

    }
    detIndex.clear();

    std::vector<std::vector<int> >& Vcopy = *var_indices;
    std::vector<std::vector<size_t> >& Ocopy = *orbDifference;

    size_t uniqueSize = 0;

    if (extra_info) {

      if (varIndicesEmtpy) {
	var_indices->push_back(std::vector<int>(1,var_indices_beforeMerge->at(0)));
	orbDifference->push_back(std::vector<size_t>(1,orbDifference_beforeMerge->at(0)));
      }
    }

    if (varIndicesEmtpy) {
      for (size_t i=1; i <Detcopy.size(); i++) {
	if (!(Detcopy[i] == Detcopy[i-1])) {
	  uniqueSize++;
	  Det->operator[](uniqueSize) = Det->at(i);
	  Num->operator[](uniqueSize) = Num->at(i);
	  if (Num2->size() != 0)
	    Num2->operator[](uniqueSize) = Num2->at(i);
	  Energy->operator[](uniqueSize) = Energy->at(i);
	  if (extra_info) {
	    var_indices->push_back(std::vector<int>(1,var_indices_beforeMerge->at(i)));
	    orbDifference->push_back(std::vector<size_t>(1,orbDifference_beforeMerge->at(i)));
	    //var_indices->operator[](uniqueSize) = Vcopy[detIndex[i]];
	    //orbDifference->operator[](uniqueSize) = Ocopy[detIndex[i]];
	  }
	}
	else {
	  Num->operator[](uniqueSize) += Num->at(i);
	  if (Num2->size() != 0)
	    Num2->operator[](uniqueSize) += Num2->at(i);
	  if (abs(Energy->operator[](uniqueSize) - Energy->at(i)) > 1.e-12) {
	    cout << uniqueSize<<"  "<<i<<"  "<<endl;
	    cout << Detcopy[i]<<endl;
	    cout << Energy->at(uniqueSize)<<"  "<<Energy->at(i)<<"  "<<Detcopy[i-1]<<endl;
	    cout << "Energy of determinants is not consistent."<<endl;
	    exit(0);
	  }
	  if (extra_info) {
	    var_indices->at(uniqueSize).push_back(var_indices_beforeMerge->at(i));
	    orbDifference->at(uniqueSize).push_back(orbDifference_beforeMerge->at(i));
    	    //(*var_indices)[uniqueSize].push_back(Vcopy[detIndex[i]][k]);
    	    //(*orbDifference)[uniqueSize].push_back(Ocopy[detIndex[i]][k]);
	  }
	}
      }
    }
    else {
      for (size_t i=1; i <Detcopy.size(); i++) {
	if (!(Detcopy[i] == Detcopy[i-1])) {
	  uniqueSize++;
	  Det->operator[](uniqueSize) = Det->at(i);
	  Num->operator[](uniqueSize) = Num->at(i);
	  if (Num2->size() != 0)
	    Num2->operator[](uniqueSize) = Num2->at(i);
	  Energy->operator[](uniqueSize) = Energy->at(i);
	  if (extra_info) {
	    var_indices->operator[](uniqueSize) = Vcopy[i];
	    orbDifference->operator[](uniqueSize) = Ocopy[i];
	  }
	}
	else {
	  Num->operator[](uniqueSize) += Num->at(i);
	  if (Num2->size() != 0)
	    Num2->operator[](uniqueSize) += Num2->at(i);
	  if (extra_info) {
	    for (size_t k=0; k<Vcopy[i].size(); k++) {
	      (*var_indices)[uniqueSize].push_back(Vcopy[i][k]);
	      (*orbDifference)[uniqueSize].push_back(Ocopy[i][k]);
	    }
	  }
	}
      }
    }


    var_indices_beforeMerge->clear();
    orbDifference_beforeMerge->clear();

    Det->resize(uniqueSize+1);
    Num->resize(uniqueSize+1);
    if (Num2->size() != 0)
      Num2->resize(uniqueSize+1);
    Energy->resize(uniqueSize+1);
    if (extra_info) {
      var_indices->resize(uniqueSize+1);
      orbDifference->resize(uniqueSize+1);
    }
  }


  void RemoveDetsPresentIn(std::vector<Determinant>& SortedDets) {
    vector<Determinant>::iterator vec_it = SortedDets.begin();
    std::vector<Determinant>& Detcopy = *Det;
    std::vector<CItype>& Numcopy = *Num;
    std::vector<CItype>& Num2copy = *Num2;
    std::vector<double>& Ecopy = *Energy;
    //if (extra_info) {
    std::vector<std::vector<int> >& Vcopy = *var_indices;
    std::vector<std::vector<size_t> >& Ocopy = *orbDifference;
    //}

    size_t uniqueSize = 0;
    for (size_t i=0; i<Detcopy.size();) {
      if (Detcopy[i] < *vec_it) {
	Det->operator[](uniqueSize) = Detcopy[i];
	Num->operator[](uniqueSize) = Numcopy[i];
	if (Num2->size() != 0)
	  Num2->operator[](uniqueSize) = Num2copy[i];
	Energy->operator[](uniqueSize) = Ecopy[i];
	if (extra_info) {
	  var_indices->operator[](uniqueSize) = Vcopy[i];
	  orbDifference->operator[](uniqueSize) = Ocopy[i];
	}
	i++; uniqueSize++;
      }
      else if (*vec_it < Detcopy[i] && vec_it != SortedDets.end())
	vec_it ++;
      else if (*vec_it < Detcopy[i] && vec_it == SortedDets.end()) {
	Det->operator[](uniqueSize) = Detcopy[i];
	Num->operator[](uniqueSize) = Numcopy[i];
	if (Num2->size() != 0)
	  Num2->operator[](uniqueSize) = Num2copy[i];
	Energy->operator[](uniqueSize) = Ecopy[i];
	if (extra_info) {
	  var_indices->operator[](uniqueSize) = Vcopy[i];
	  orbDifference->operator[](uniqueSize) = Ocopy[i];
	}
	i++; uniqueSize++;
      }
      else {
	vec_it++; i++;
      }
    }
    Det->resize(uniqueSize); Num->resize(uniqueSize); 
    if (Num2->size() != 0)
      Num2->resize(uniqueSize); 
    Energy->resize(uniqueSize);
    if (extra_info) {
      var_indices->resize(uniqueSize); orbDifference->resize(uniqueSize);
    }
  }

  void RemoveDuplicates() {
    std::vector<Determinant>& Detcopy = *Det;
    std::vector<CItype>& Numcopy = *Num;
    std::vector<double>& Ecopy = *Energy;
    //if (extra_info) {
    std::vector<std::vector<int> >& Vcopy = *var_indices;
    std::vector<std::vector<size_t> >& Ocopy = *orbDifference;
    //}

    if (Det->size() <= 1) return;
    //if (Det->size() == 1) return;
    size_t uniqueSize = 0;
    for (size_t i=1; i <Detcopy.size(); i++) {
      if (!(Detcopy[i] == Detcopy[i-1])) {
	uniqueSize++;
	Det->operator[](uniqueSize) = Detcopy[i];
	Num->operator[](uniqueSize) = Numcopy[i];
	if (Num2->size() != 0)
	  Num2->operator[](uniqueSize) = Num2->operator[](i);
	Energy->operator[](uniqueSize) = Ecopy[i];
	if (extra_info) {
	  var_indices->operator[](uniqueSize) = Vcopy[i];
	  orbDifference->operator[](uniqueSize) = Ocopy[i];
	}
      }
      else {// Same det, so combine
	Num->operator[](uniqueSize) += Numcopy[i];
	if (Num2->size() != 0)
	  Num2->operator[](uniqueSize) += Num2->operator[](i);
	//Num2->operator[](uniqueSize) += Num2copy[i];
	if (extra_info) {
	  for (size_t k=0; k<(*var_indices)[i].size(); k++) {
	    (*var_indices)[uniqueSize].push_back((*var_indices)[i][k]);
	    (*orbDifference)[uniqueSize].push_back((*orbDifference)[i][k]);
	  }
	}
      }
    }
    Det->resize(uniqueSize+1);
    Num->resize(uniqueSize+1);
    if (Num2->size() != 0)
      Num2->resize(uniqueSize+1);
    Energy->resize(uniqueSize+1);
    if (extra_info) {
      var_indices->resize(uniqueSize+1);
      orbDifference->resize(uniqueSize+1);
    }
  }

  void deepCopy(const StitchDEH& s) {
    *Det = *(s.Det);
    *Num = *(s.Num);
    if (s.Num2->size() != 0)
      *Num2 = *(s.Num2);
    *Energy = *(s.Energy);
    if (extra_info) {
      *var_indices = *(s.var_indices);
      *orbDifference = *(s.orbDifference);
    }
  }

  void operator=(const StitchDEH& s) {
    Det = s.Det;
    Num = s.Num;
    Num2 = s.Num2;
    Energy = s.Energy;
    if (extra_info) {
      var_indices = s.var_indices;
      orbDifference = s.orbDifference;
    }
  }

  void clear() {
    Det->clear();
    Num->clear();
    Num2->clear();
    Energy->clear();
    if (extra_info) {
      var_indices->clear();
      orbDifference->clear();
    }
  }

  void resize(size_t s) {
    Det->resize(s);
    Num->resize(s);
    Num2->resize(2);
    Energy->resize(s);
  }

  void merge(const StitchDEH& s) {
    // Merges with disjoint set
    std::vector<Determinant> Detcopy = *Det;
    std::vector<CItype> Numcopy = *Num;
    std::vector<CItype> Num2copy = *Num2;
    std::vector<double> Ecopy = *Energy;
    //if (extra_info) {
    std::vector<std::vector<int> > Vcopy = *var_indices;
    std::vector<std::vector<size_t> > Ocopy = *orbDifference;
    //}

    Det->resize(Detcopy.size()+s.Det->size());
    Num->resize(Numcopy.size()+s.Det->size());
    Num2->resize(Num2copy.size()+s.Num2->size());
    Energy->resize(Ecopy.size()+s.Energy->size());
    if (extra_info) {
      var_indices->resize(Vcopy.size()+s.var_indices->size());
      orbDifference->resize(Ocopy.size()+s.orbDifference->size());
    }

    //cout << Det->size()<<"  "<<Detcopy.size()<<"  "<<s.Det->size()<<endl;
    size_t j = 0, k=0,l=0;
    while (j<Detcopy.size() && k <s.Det->size()) {
      if (Detcopy.operator[](j) < s.Det->operator[](k)) {
	Det->operator[](l) = Detcopy.operator[](j);
	Num->operator[](l) = Numcopy.operator[](j);
	if (Num2->size() != 0)
	  Num2->operator[](l) = Num2copy.operator[](j);
	Energy->operator[](l) = Ecopy.operator[](j);
        if (extra_info) {
	  var_indices->operator[](l) = Vcopy.operator[](j);
	  orbDifference->operator[](l) = Ocopy.operator[](j);
        }
	j++; l++;
      }
      else {
	Det->operator[](l) = s.Det->operator[](k);
	Num->operator[](l) = s.Num->operator[](k);
	if (Num2->size() != 0)
	  Num2->operator[](l) = Num2copy.operator[](j);
	Energy->operator[](l) = s.Energy->operator[](k);
        if (extra_info) {
	  var_indices->operator[](l) = s.var_indices->operator[](k);
	  orbDifference->operator[](l) = s.orbDifference->operator[](k);
        }
	k++;l++;
      }
    }
    while (j<Detcopy.size()) {
      Det->operator[](l) = Detcopy.operator[](j);
      Num->operator[](l) = Numcopy.operator[](j);
      if (Num2->size() != 0)
	Num2->operator[](l) = Num2copy.operator[](j);
      Energy->operator[](l) = Ecopy.operator[](j);
      if (extra_info) {
        var_indices->operator[](l) = Vcopy.operator[](j);
        orbDifference->operator[](l) = Ocopy.operator[](j);
      }
      j++; l++;
    }
    while (k<s.Det->size()) {
      Det->operator[](l) = s.Det->operator[](k);
      Num->operator[](l) = s.Num->operator[](k);
      if (Num2->size() != 0)
	Num2->operator[](l) = Num2copy.operator[](j);
      Energy->operator[](l) = s.Energy->operator[](k);
      if (extra_info) {
        var_indices->operator[](l) = s.var_indices->operator[](k);
        orbDifference->operator[](l) = s.orbDifference->operator[](k);
      }
      k++;l++;
    }

  } // end merge

};

class ElementWiseAddStitchDEH {
public:
  StitchDEH operator()(const StitchDEH& s1, const StitchDEH& s2) {
    StitchDEH out;
    out.deepCopy(s1);
    out.merge(s2);
    return out;
  }
};





void populateSpatialRDM(int& i, int& j, int& k, int& l, MatrixXx& s2RDM, CItype value, int& nSpatOrbs) {
  //we assume i != j  and  k != l
  int I = i/2, J=j/2, K=k/2, L=l/2;
  if (i%2 == l%2 && j%2 == k%2) {
    s2RDM(I*nSpatOrbs+J,L*nSpatOrbs+K) -= value;
    s2RDM(J*nSpatOrbs+I,K*nSpatOrbs+L) -= value;
  }

  if (i%2 == k%2 && l%2 == j%2 ) {
    s2RDM(I*nSpatOrbs+J,K*nSpatOrbs+L) += value;
    s2RDM(J*nSpatOrbs+I,L*nSpatOrbs+K) += value;
  }

}

void SHCIbasics::EvaluateAndStoreRDM(vector<vector<int> >& connections, vector<Determinant>& Dets, MatrixXx& ci,
				    vector<vector<size_t> >& orbDifference, int nelec, schedule& schd, int root, bool update) {
  boost::mpi::communicator world;

  size_t norbs = Dets[0].norbs;
  int nSpatOrbs = norbs/2;

  //RDM(i,j,k,l) = a_i^\dag a_j^\dag a_l a_k
  //also i>=j and k>=l
  MatrixXx twoRDM;
  if (schd.DoSpinRDM ){
    if (update) {
      if (mpigetrank() == 0) {
	char file [5000];
	sprintf (file, "%s/%d-spinRDM.bkp" , schd.prefix[0].c_str(), root );
	std::ifstream ifs(file, std::ios::binary);
	boost::archive::binary_iarchive load(ifs);
	load >> twoRDM;
      }
      mpi::broadcast(world, twoRDM, 0);
    }
    else
      twoRDM.setZero(norbs*(norbs+1)/2, norbs*(norbs+1)/2);
  }

  MatrixXx s2RDM;
  if (update) {
    if (mpigetrank() == 0) {
      char file [5000];
      sprintf (file, "%s/%d-spatialRDM.bkp" , schd.prefix[0].c_str(), root );
      std::ifstream ifs(file, std::ios::binary);
      boost::archive::binary_iarchive load(ifs);
      load >> s2RDM;
    }
    mpi::broadcast(world, s2RDM, 0);
  }
  else
    s2RDM.setZero(nSpatOrbs*nSpatOrbs, nSpatOrbs*nSpatOrbs);

  int num_thrds = omp_get_max_threads();

  //#pragma omp parallel for schedule(dynamic)
  for (int i=0; i<Dets.size(); i++) {
    if ((i/num_thrds)%world.size() != world.rank()) continue;

    vector<int> closed(nelec, 0);
    vector<int> open(norbs-nelec,0);
    Dets[i].getOpenClosed(open, closed);

    //<Di| Gamma |Di>
    for (int n1=0; n1<nelec; n1++) {
      for (int n2=0; n2<n1; n2++) {
	int orb1 = closed[n1], orb2 = closed[n2];
	if (schd.DoSpinRDM)
	  twoRDM(orb1*(orb1+1)/2 + orb2, orb1*(orb1+1)/2+orb2) += ci(i,0)*ci(i,0);
	populateSpatialRDM(orb1, orb2, orb1, orb2, s2RDM, ci(i,0)*ci(i,0), nSpatOrbs);
      }
    }


    for (int j=1; j<connections[i].size(); j++) {
      int d0=orbDifference[i][j]%norbs, c0=(orbDifference[i][j]/norbs)%norbs ;

      if (orbDifference[i][j]/norbs/norbs == 0) { //only single excitation
	for (int n1=0;n1<nelec; n1++) {
	  double sgn = 1.0;
	  int a=max(closed[n1],c0), b=min(closed[n1],c0), I=max(closed[n1],d0), J=min(closed[n1],d0);
	  if (closed[n1] == d0) continue;
	  Dets[i].parity(min(d0,c0), max(d0,c0),sgn);
	  if (!( (closed[n1] > c0 && closed[n1] > d0) || (closed[n1] < c0 && closed[n1] < d0))) sgn *=-1.;
	  if (schd.DoSpinRDM) {
	    twoRDM(a*(a+1)/2+b, I*(I+1)/2+J) += sgn*ci(connections[i][j],0)*ci(i,0);
	    twoRDM(I*(I+1)/2+J, a*(a+1)/2+b) += sgn*ci(connections[i][j],0)*ci(i,0);
	  }

	  populateSpatialRDM(a, b, I, J, s2RDM, sgn*ci(connections[i][j],0)*ci(i,0), nSpatOrbs);
	  populateSpatialRDM(I, J, a, b, s2RDM, sgn*ci(connections[i][j],0)*ci(i,0), nSpatOrbs);

	}
      }
      else {
	int d1=(orbDifference[i][j]/norbs/norbs)%norbs, c1=(orbDifference[i][j]/norbs/norbs/norbs)%norbs ;
	double sgn = 1.0;

	Dets[i].parity(d1,d0,c1,c0,sgn);

	if (schd.DoSpinRDM) {
	  twoRDM(c1*(c1+1)/2+c0, d1*(d1+1)/2+d0) += sgn*ci(connections[i][j],0)*ci(i,0);
	  twoRDM(d1*(d1+1)/2+d0, c1*(c1+1)/2+c0) += sgn*ci(connections[i][j],0)*ci(i,0);
	}

	populateSpatialRDM(c1, c0, d1, d0, s2RDM, sgn*ci(connections[i][j],0)*ci(i,0), nSpatOrbs);
	populateSpatialRDM(d1, d0, c1, c0, s2RDM, sgn*ci(connections[i][j],0)*ci(i,0), nSpatOrbs);
      }
    }
  }

  if (schd.DoSpinRDM)
    MPI_Allreduce(MPI_IN_PLACE, &twoRDM(0,0), twoRDM.rows()*twoRDM.cols(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  MPI_Allreduce(MPI_IN_PLACE, &s2RDM(0,0), s2RDM.rows()*s2RDM.cols(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

  if(mpigetrank() == 0) {
    char file [5000];
    sprintf (file, "%s/spatialRDM.%d.%d.txt" , schd.prefix[0].c_str(), root, root );
    std::ofstream ofs(file, std::ios::out);
    ofs << nSpatOrbs<<endl;

    for (int n1=0; n1<nSpatOrbs; n1++)
      for (int n2=0; n2<nSpatOrbs; n2++)
	for (int n3=0; n3<nSpatOrbs; n3++)
	  for (int n4=0; n4<nSpatOrbs; n4++)
	    {
	      if (abs(s2RDM(n1*nSpatOrbs+n2, n3*nSpatOrbs+n4))  > 1.e-6)
		ofs << str(boost::format("%3d   %3d   %3d   %3d   %10.8g\n") % n1 % n2 % n3 % n4 % s2RDM(n1*nSpatOrbs+n2, n3*nSpatOrbs+n4));
	    }
    ofs.close();



    if (schd.DoSpinRDM) {
      char file [5000];
      sprintf (file, "%s/%d-spinRDM.bkp" , schd.prefix[0].c_str(), root );
      std::ofstream ofs(file, std::ios::binary);
      boost::archive::binary_oarchive save(ofs);
      save << twoRDM;
    }

    {
      char file [5000];
      sprintf (file, "%s/%d-spatialRDM.bkp" , schd.prefix[0].c_str(), root );
      std::ofstream ofs(file, std::ios::binary);
      boost::archive::binary_oarchive save(ofs);
      save << s2RDM;
    }
  }

}



void SHCIbasics::ComputeEnergyFromSpinRDM(int norbs, int nelec, oneInt& I1, twoInt& I2, double coreE, MatrixXx& twoRDM) {

  //RDM(i,j,k,l) = a_i^\dag a_j^\dag a_l a_k
  //also i>=j and k>=l
  double energy = coreE;
  double onebody = 0.0;
  double twobody = 0.0;
  //if (mpigetrank() == 0)  cout << "Core energy= " << energy << endl;

  MatrixXx oneRDM = MatrixXx::Zero(norbs, norbs);
#pragma omp parallel for schedule(dynamic)
  for (int p=0; p<norbs; p++)
    for (int q=0; q<norbs; q++)
      for (int r=0; r<norbs; r++) {
	int P = max(p,r), R1 = min(p,r);
	int Q = max(q,r), R2 = min(q,r);
	double sgn = 1.;
	if (P != p)  sgn *= -1;
	if (Q != q)  sgn *= -1;

	oneRDM(p,q) += sgn*twoRDM(P*(P+1)/2+R1,Q*(Q+1)/2+R2)/(nelec-1.);
      }

#pragma omp parallel for reduction(+ : onebody)
  for (int p=0; p<norbs; p++)
    for (int q=0; q<norbs; q++)
#ifdef Complex
      onebody += (I1(p, q)*oneRDM(p,q)).real();
#else
  onebody += I1(p, q)*oneRDM(p,q);
#endif

#pragma omp parallel for reduction(+ : twobody)
  for (int p=0; p<norbs; p++){
    for (int q=0; q<norbs; q++){
      for (int r=0; r<norbs; r++){
	for (int s=0; s<norbs; s++){
	  //if (p%2 != r%2 || q%2 != s%2)  continue; // This line is not necessary
	  int P = max(p,q), Q = min(p,q);
	  int R = max(r,s), S = min(r,s);
	  double sgn = 1;
	  if (P != p)  sgn *= -1;
	  if (R != r)  sgn *= -1;
#ifdef Complex
	  twobody += (sgn * 0.5 * twoRDM(P*(P+1)/2+Q, R*(R+1)/2+S) * I2(p,r,q,s)).real(); // 2-body term
#else
	  twobody += sgn * 0.5 * twoRDM(P*(P+1)/2+Q, R*(R+1)/2+S) * I2(p,r,q,s); // 2-body term
#endif
	}
      }
    }
  }

  //if (mpigetrank() == 0)  cout << "One-body from 2RDM: " << onebody << endl;
  //if (mpigetrank() == 0)  cout << "Two-body from 2RDM: " << twobody << endl;

  energy += onebody + twobody;
  if (mpigetrank() == 0)  cout << "E from 2RDM: " << energy << endl;

}


void SHCIbasics::ComputeEnergyFromSpatialRDM(int norbs, int nelec, oneInt& I1, twoInt& I2, double coreE, MatrixXx& twoRDM) {

  double energy = coreE;
  double onebody = 0.0;
  double twobody = 0.0;

  MatrixXx oneRDM = MatrixXx::Zero(norbs, norbs);
#pragma omp parallel for schedule(dynamic)
  for (int p=0; p<norbs; p++)
    for (int q=0; q<norbs; q++)
      for (int r=0; r<norbs; r++)
	oneRDM(p,q) += twoRDM(p*norbs+r, q*norbs+ r)/(1.*nelec-1.);


#pragma omp parallel for reduction(+ : onebody)
  for (int p=0; p<norbs; p++)
    for (int q=0; q<norbs; q++)
#ifdef Complex
      onebody += (I1(2*p, 2*q)*oneRDM(p,q)).real();
#else
  onebody += I1(2*p, 2*q)*oneRDM(p,q);
#endif

#pragma omp parallel for reduction(+ : twobody)
  for (int p=0; p<norbs; p++)
    for (int q=0; q<norbs; q++)
      for (int r=0; r<norbs; r++)
	for (int s=0; s<norbs; s++)
#ifdef Complex
	  twobody +=  (0.5 * twoRDM(p*norbs+q,r*norbs+s) * I2(2*p,2*r,2*q,2*s)).real(); // 2-body term
#else
  twobody +=  0.5*twoRDM(p*norbs+q,r*norbs+s) * I2(2*p,2*r,2*q,2*s); // 2-body term
#endif

  energy += onebody + twobody;
  if (mpigetrank() == 0)  cout << "E from 2RDM: " << energy << endl;

}



void SHCIbasics::printRDM(int norbs, schedule& schd, int root, MatrixXx& twoRDM) {

  boost::mpi::communicator world;

  //RDM(i,j,k,l) = a_i^\dag a_j^\dag a_l a_k
  //also i>=j and k>=l
  int num_thrds = omp_get_max_threads();

  int nSpatOrbs = norbs/2;


  if(mpigetrank() == 0) {
    MatrixXx s2RDM(nSpatOrbs*nSpatOrbs, nSpatOrbs*nSpatOrbs);
    s2RDM *= 0.0;

    char file [5000];
    sprintf (file, "%s/spatialRDM.%d.%d.txt" , schd.prefix[0].c_str(), root, root );
    std::ofstream ofs(file, std::ios::out);
    ofs << nSpatOrbs<<endl;

#pragma omp parallel for schedule(static)
    for (int n1=0; n1<nSpatOrbs; n1++)
      for (int n2=0; n2<nSpatOrbs; n2++)
	for (int n3=0; n3<nSpatOrbs; n3++)
	  for (int n4=0; n4<nSpatOrbs; n4++)
	    {
	      double sgn = 1.0;
	      int N1 = 2*max(n1,n2), N2=2*min(n1,n2), N3=2*max(n3,n4), N4=2*min(n3,n4);
	      if(( (n1>=n2 && n3<n4) || (n1<n2 && n3>=n4))) sgn = -1.0;
	      s2RDM(n1*nSpatOrbs+n2, n3*nSpatOrbs+n4) += sgn*twoRDM( N1*(N1+1)/2+N2, N3*(N3+1)/2+N4);

	      sgn = 1.0;
	      N1 = max(2*n1+1,2*n2); N2=min(2*n1+1,2*n2); N3=max(2*n3+1,2*n4); N4=min(2*n3+1,2*n4);
	      if(!( (2*n1+1>2*n2 && 2*n3+1>2*n4) || (2*n1+1<2*n2 && 2*n3+1<2*n4))) sgn = -1.0;
	      s2RDM(n1*nSpatOrbs+n2, n3*nSpatOrbs+n4) += sgn*twoRDM( N1*(N1+1)/2+N2, N3*(N3+1)/2+N4);

	      sgn = 1.0;
	      N1 = max(2*n1,2*n2+1); N2=min(2*n1,2*n2+1); N3=max(2*n3,2*n4+1); N4=min(2*n3,2*n4+1);
	      if(!( (2*n1>=2*n2+1 && 2*n3>=2*n4+1) || (2*n1<2*n2+1 && 2*n3<2*n4+1))) sgn = -1.0;
	      s2RDM(n1*nSpatOrbs+n2, n3*nSpatOrbs+n4) += sgn*twoRDM( N1*(N1+1)/2+N2, N3*(N3+1)/2+N4);

	      sgn = 1.0;
	      N1 = 2*max(n1,n2)+1; N2=2*min(n1,n2)+1; N3=2*max(n3,n4)+1; N4=2*min(n3,n4)+1;
	      if(( (n1>=n2 && n3<n4) || (n1<n2 && n3>=n4))) sgn = -1.0;
	      s2RDM(n1*nSpatOrbs+n2, n3*nSpatOrbs+n4) += sgn*twoRDM( N1*(N1+1)/2+N2, N3*(N3+1)/2+N4);

	    }

    for (int n1=0; n1<nSpatOrbs; n1++)
      for (int n2=0; n2<nSpatOrbs; n2++)
	for (int n3=0; n3<nSpatOrbs; n3++)
	  for (int n4=0; n4<nSpatOrbs; n4++)
	    {
	      if (abs(s2RDM(n1*nSpatOrbs+n2, n3*nSpatOrbs+n4))  > 1.e-6)
		ofs << str(boost::format("%3d   %3d   %3d   %3d   %10.8g\n") % n1 % n2 % n3 % n4 % s2RDM(n1*nSpatOrbs+n2, n3*nSpatOrbs+n4));
	    }



    {
      char file [5000];
      sprintf (file, "%s/%d-spinRDM.bkp" , schd.prefix[0].c_str(), root );
      std::ofstream ofs(file, std::ios::binary);
      boost::archive::binary_oarchive save(ofs);
      save << twoRDM;
    }

    {
      char file [5000];
      sprintf (file, "%s/%d-spatialRDM.bkp" , schd.prefix[0].c_str(), root );
      std::ofstream ofs(file, std::ios::binary);
      boost::archive::binary_oarchive save(ofs);
      save << s2RDM;
    }
  }
}


void SHCIbasics::DoPerturbativeStochastic2SingleListDoubleEpsilon2(vector<Determinant>& Dets, MatrixXx& ci, double& E0, oneInt& I1, twoInt& I2,
								  twoIntHeatBathSHM& I2HB, vector<int>& irrep, schedule& schd, double coreE, int nelec, int root) {

  boost::mpi::communicator world;
  char file [5000];
  sprintf (file, "output-%d.bkp" , world.rank() );
  std::ofstream ofs;
  if (root == 0)
    ofs.open(file, std::ofstream::out);
  else
    ofs.open(file, std::ofstream::app);

  double epsilon2 = schd.epsilon2;
  schd.epsilon2 = schd.epsilon2Large;
  double EptLarge = DoPerturbativeDeterministic(Dets, ci, E0, I1, I2, I2HB, irrep, schd, coreE, nelec, root);

  schd.epsilon2 = epsilon2;

  int norbs = Determinant::norbs;
  std::vector<Determinant> SortedDets = Dets; std::sort(SortedDets.begin(), SortedDets.end());
  int niter = schd.nPTiter;
  //double eps = 0.001;
  int Nsample = schd.SampleN;
  double AvgenergyEN = 0.0;
  double AverageDen = 0.0;
  int currentIter = 0;
  int sampleSize = 0;
  int num_thrds = omp_get_max_threads();

  double cumulative = 0.0;
  for (int i=0; i<ci.rows(); i++)
    cumulative += abs(ci(i,0));

  std::vector<int> alias; std::vector<double> prob;
  SHCIsampledeterminants::setUpAliasMethod(ci, cumulative, alias, prob);
#pragma omp parallel for schedule(dynamic)
  for (int iter=0; iter<niter; iter++) {
    //cout << norbs<<"  "<<nelec<<endl;
    char psiArray[norbs];
    vector<int> psiClosed(nelec,0);
    vector<int> psiOpen(norbs-nelec,0);
    //char psiArray[norbs];
    std::vector<CItype> wts1(Nsample,0.0); std::vector<int> Sample1(Nsample,-1);

    //int Nmc = sample_N2(ci, cumulative, Sample1, wts1);
    int distinctSample = SHCIsampledeterminants::sample_N2_alias(ci, cumulative, Sample1, wts1, alias, prob);
    int Nmc = Nsample;
    double norm = 0.0;

    //map<Determinant, std::tuple<double,double,double, double, double> > Psi1ab;
    std::vector<Determinant> Psi1; std::vector<CItype>  numerator1A; vector<double> numerator2A;
    vector<char> present;
    std::vector<double>  det_energy;
    for (int i=0; i<distinctSample; i++) {
      int I = Sample1[i];
      SHCIgetdeterminants::getDeterminantsStochastic2Epsilon(Dets[I], schd.epsilon2/abs(ci(I,0)), schd.epsilon2Large/abs(ci(I,0)), wts1[i], ci(I,0), I1, I2, I2HB, irrep, coreE, E0, Psi1, numerator1A, numerator2A, present, det_energy, schd, Nmc, nelec);
    }


    quickSort( &(Psi1[0]), 0, Psi1.size(), &numerator1A[0], &numerator2A[0], &det_energy, &present);

    CItype currentNum1A=0.; double currentNum2A=0.;
    CItype currentNum1B=0.; double currentNum2B=0.;
    vector<Determinant>::iterator vec_it = SortedDets.begin();
    double energyEN = 0.0, energyENLargeEps = 0.0;

    for (int i=0;i<Psi1.size();) {
      if (Psi1[i] < *vec_it) {
	currentNum1A += numerator1A[i];
	currentNum2A += numerator2A[i];
	if (present[i]) {
	  currentNum1B += numerator1A[i];
	  currentNum2B += numerator2A[i];
	}

	if ( i == Psi1.size()-1) {
	  energyEN += (pow(abs(currentNum1A),2)*Nmc/(Nmc-1) - currentNum2A)/(det_energy[i] - E0);
	  energyENLargeEps += (pow(abs(currentNum1B),2)*Nmc/(Nmc-1) - currentNum2B)/(det_energy[i] - E0);
	}
	else if (!(Psi1[i] == Psi1[i+1])) {
	  energyEN += ( pow(abs(currentNum1A),2)*Nmc/(Nmc-1) - currentNum2A)/(det_energy[i] - E0);
	  energyENLargeEps += ( pow(abs(currentNum1B),2)*Nmc/(Nmc-1) - currentNum2B)/(det_energy[i] - E0);
	  currentNum1A = 0.;
	  currentNum2A = 0.;
	  currentNum1B = 0.;
	  currentNum2B = 0.;
	}
	i++;
      }
      else if (*vec_it <Psi1[i] && vec_it != SortedDets.end())
	vec_it++;
      else if (*vec_it <Psi1[i] && vec_it == SortedDets.end()) {
	currentNum1A += numerator1A[i];
	currentNum2A += numerator2A[i];
	if (present[i]) {
	  currentNum1B += numerator1A[i];
	  currentNum2B += numerator2A[i];
	}

	if ( i == Psi1.size()-1) {
	  energyEN += ( pow(abs(currentNum1A),2)*Nmc/(Nmc-1) - currentNum2A)/(det_energy[i] - E0);
	  energyENLargeEps += ( pow(abs(currentNum1B),2)*Nmc/(Nmc-1) - currentNum2B)/(det_energy[i] - E0);
	}
	else if (!(Psi1[i] == Psi1[i+1])) {
	  energyEN += ( pow(abs(currentNum1A),2)*Nmc/(Nmc-1) - currentNum2A)/(det_energy[i] - E0);
	  energyENLargeEps += ( pow(abs(currentNum1B),2)*Nmc/(Nmc-1) - currentNum2B)/(det_energy[i] - E0);
	  //energyEN += (currentNum1A*currentNum1A*Nmc/(Nmc-1) - currentNum2A)/(det_energy[i] - E0);
	  //energyENLargeEps += (currentNum1B*currentNum1B*Nmc/(Nmc-1) - currentNum2B)/(det_energy[i] - E0);
	  currentNum1A = 0.;
	  currentNum2A = 0.;
	  currentNum1B = 0.;
	  currentNum2B = 0.;
	}
	i++;
      }
      else {
	if (Psi1[i] == Psi1[i+1])
	  i++;
	else {
	  vec_it++; i++;
	}
      }
    }

    sampleSize = distinctSample;


#pragma omp critical
    {
      if (mpigetrank() == 0) {
	AvgenergyEN += -energyEN+energyENLargeEps+EptLarge; currentIter++;
	std::cout << format("%6i  %14.8f  %s%i %14.8f   %10.2f  %10i %4i")
	  %(currentIter) % (E0-energyEN+energyENLargeEps+EptLarge) % ("Root") % root % (E0+AvgenergyEN/currentIter) % (getTime()-startofCalc) % sampleSize % (omp_get_thread_num());
	cout << endl;
      }
      else {
	AvgenergyEN += -energyEN+energyENLargeEps+EptLarge; currentIter++;
	ofs << format("%6i  %14.8f  %s%i %14.8f   %10.2f  %10i %4i")
	  %(currentIter) % (E0-energyEN+energyENLargeEps+EptLarge) % ("Root") % root % (E0+AvgenergyEN/currentIter) % (getTime()-startofCalc) % sampleSize % (omp_get_thread_num());
	ofs << endl;

      }
    }
  }
  ofs.close();

}




void SHCIbasics::DoPerturbativeStochastic2SingleListDoubleEpsilon2OMPTogether(vector<Determinant>& Dets, MatrixXx& ci, double& E0, oneInt& I1, twoInt& I2,
									     twoIntHeatBathSHM& I2HB, vector<int>& irrep, schedule& schd, double coreE, int nelec, int root) {

  boost::mpi::communicator world;
  char file [5000];
  sprintf (file, "output-%d.bkp" , world.rank() );
  std::ofstream ofs;
  if (root == 0)
    ofs.open(file, std::ofstream::out);
  else
    ofs.open(file, std::ofstream::app);

  double epsilon2 = schd.epsilon2;
  schd.epsilon2 = schd.epsilon2Large;
  double EptLarge = DoPerturbativeDeterministic(Dets, ci, E0, I1, I2, I2HB, irrep, schd, coreE, nelec, root);

  schd.epsilon2 = epsilon2;

  int norbs = Determinant::norbs;
  std::vector<Determinant> SortedDets = Dets; std::sort(SortedDets.begin(), SortedDets.end());
  int niter = schd.nPTiter;
  //double eps = 0.001;
  int Nsample = schd.SampleN;
  double AvgenergyEN = 0.0;
  double AverageDen = 0.0;
  int currentIter = 0;
  int sampleSize = 0;
  int num_thrds = omp_get_max_threads();

  double cumulative = 0.0;
  for (int i=0; i<ci.rows(); i++)
    cumulative += abs(ci(i,0));

  std::vector<int> alias; std::vector<double> prob;
  SHCIsampledeterminants::setUpAliasMethod(ci, cumulative, alias, prob);

  double totalPT=0, totalPTLargeEps=0;

  std::vector< std::vector<vector<Determinant> > > hashedDetBeforeMPI   (num_thrds);// vector<Determinant> > >(num_thrds));
  std::vector< std::vector<vector<CItype> > >      hashedNum1BeforeMPI  (num_thrds);// std::vector<vector<double> > >(num_thrds));
  std::vector< std::vector<vector<double> > >      hashedNum2BeforeMPI  (num_thrds);// std::vector<vector<double> > >(num_thrds));
  std::vector< std::vector<vector<double> > >      hashedEnergyBeforeMPI(num_thrds);// std::vector<vector<double> > >(num_thrds));
  std::vector< std::vector<vector<char> > >        hashedpresentBeforeMPI(num_thrds);// std::vector<vector<char> > >(num_thrds));

  int AllDistinctSample = 0;
  int Nmc = Nsample*num_thrds;
  std::vector<CItype> allwts(Nmc, 0.0); std::vector<int> allSample(Nmc, -1);

#pragma omp parallel
  {
    for (int iter=0; iter<niter; iter++) {
      std::vector<CItype> wts1(Nsample,0.0); std::vector<int> Sample1(Nsample,-1);
      int distinctSample = 0;

      if (omp_get_thread_num() == 0) {
	std::fill(allSample.begin(), allSample.end(), -1);
	AllDistinctSample = SHCIsampledeterminants::sample_N2_alias(ci, cumulative, allSample, allwts, alias, prob);
      }
#pragma omp barrier
      if (omp_get_thread_num() <  AllDistinctSample%num_thrds)
	distinctSample = AllDistinctSample/num_thrds + 1;
      else
	distinctSample = AllDistinctSample/num_thrds;


      size_t stride = omp_get_thread_num() < AllDistinctSample%num_thrds \
					     ? omp_get_thread_num()*distinctSample \
					     : omp_get_thread_num()*distinctSample + AllDistinctSample%num_thrds;

      for (int i = 0; i < distinctSample; i++) {
	wts1   [i] = allwts   [i + stride];
	Sample1[i] = allSample[i + stride];
      }

      double norm = 0.0;

      std::vector<Determinant> Psi1; std::vector<CItype>  numerator1A; vector<double> numerator2A;
      vector<char> present;
      std::vector<double>  det_energy;

      for (int i=0; i<distinctSample; i++) {
	int I = Sample1[i];
	SHCIgetdeterminants::getDeterminantsStochastic2Epsilon(Dets[I], schd.epsilon2/abs(ci(I,0)),
						      schd.epsilon2Large/abs(ci(I,0)), wts1[i],
						      ci(I,0), I1, I2, I2HB, irrep, coreE, E0,
						      Psi1,
						      numerator1A,
						      numerator2A,
						      present,
						      det_energy,
						      schd, Nmc, nelec);
      }

      if(num_thrds >1) {
	hashedDetBeforeMPI[omp_get_thread_num()].resize(num_thrds);
	hashedNum1BeforeMPI[omp_get_thread_num()].resize(num_thrds);
	hashedNum2BeforeMPI[omp_get_thread_num()].resize(num_thrds);
	hashedEnergyBeforeMPI[omp_get_thread_num()].resize(num_thrds);
	hashedpresentBeforeMPI[omp_get_thread_num()].resize(num_thrds);

	for (int thrd=0; thrd<num_thrds; thrd++) {
	  hashedDetBeforeMPI[omp_get_thread_num()][thrd].reserve(Psi1.size()/num_thrds);
	  hashedNum1BeforeMPI[omp_get_thread_num()][thrd].reserve(Psi1.size()/num_thrds);
	  hashedNum2BeforeMPI[omp_get_thread_num()][thrd].reserve(Psi1.size()/num_thrds);
	  hashedEnergyBeforeMPI[omp_get_thread_num()][thrd].reserve(Psi1.size()/num_thrds);
	  hashedpresentBeforeMPI[omp_get_thread_num()][thrd].reserve(Psi1.size()/num_thrds);
	}

	for (int j=0; j<Psi1.size(); j++) {
	  size_t lOrder = Psi1.at(j).getHash();
	  size_t thrd = lOrder%(num_thrds);
	  hashedDetBeforeMPI[omp_get_thread_num()][thrd].push_back(Psi1.at(j));
	  hashedNum1BeforeMPI[omp_get_thread_num()][thrd].push_back(numerator1A.at(j));
	  hashedNum2BeforeMPI[omp_get_thread_num()][thrd].push_back(numerator2A.at(j));
	  hashedEnergyBeforeMPI[omp_get_thread_num()][thrd].push_back(det_energy.at(j));
	  hashedpresentBeforeMPI[omp_get_thread_num()][thrd].push_back(present.at(j));
	}
	Psi1.clear(); numerator1A.clear(); numerator2A.clear(); det_energy.clear(); present.clear();

#pragma omp barrier
	size_t totalSize = 0;
	for (int thrd=0; thrd<num_thrds; thrd++)
	  totalSize += hashedDetBeforeMPI[thrd][omp_get_thread_num()].size();

	Psi1.reserve(totalSize);
	numerator1A.reserve(totalSize);
	numerator2A.reserve(totalSize);
	det_energy.reserve(totalSize);
	present.reserve(totalSize);
	for (int thrd=0; thrd<num_thrds; thrd++) {
	  for (int j=0; j<hashedDetBeforeMPI[thrd][omp_get_thread_num()].size(); j++) {
	    Psi1.push_back(hashedDetBeforeMPI[thrd][omp_get_thread_num()].at(j));
	    numerator1A.push_back(hashedNum1BeforeMPI[thrd][omp_get_thread_num()].at(j));
	    numerator2A.push_back(hashedNum2BeforeMPI[thrd][omp_get_thread_num()].at(j));
	    det_energy.push_back(hashedEnergyBeforeMPI[thrd][omp_get_thread_num()].at(j));
	    present.push_back(hashedpresentBeforeMPI[thrd][omp_get_thread_num()].at(j));
	  }
	  hashedDetBeforeMPI[thrd][omp_get_thread_num()].clear();
	  hashedNum1BeforeMPI[thrd][omp_get_thread_num()].clear();
	  hashedNum2BeforeMPI[thrd][omp_get_thread_num()].clear();
	  hashedEnergyBeforeMPI[thrd][omp_get_thread_num()].clear();
	  hashedpresentBeforeMPI[thrd][omp_get_thread_num()].clear();
	}
      }

      std::vector<Determinant> Psi1copy = Psi1;
      vector<long> detIndex(Psi1.size(), 0);
      vector<long> detIndexcopy(Psi1.size(), 0);
      for (size_t i=0; i<Psi1.size(); i++)
	detIndex[i] = i;
      mergesort(&Psi1copy[0], 0, Psi1.size()-1, &detIndex[0], &( Psi1.operator[](0)), &detIndexcopy[0]);
      detIndexcopy.clear(); Psi1copy.clear();
      reorder(numerator1A, detIndex);
      reorder(numerator2A, detIndex);
      reorder(det_energy, detIndex);
      reorder(present, detIndex);
      detIndex.clear();


      CItype currentNum1A=0.; double currentNum2A=0.;
      CItype currentNum1B=0.; double currentNum2B=0.;
      vector<Determinant>::iterator vec_it = SortedDets.begin();
      double energyEN = 0.0, energyENLargeEps = 0.0;
      //size_t effNmc = num_thrds*Nmc;

      for (int i=0;i<Psi1.size();) {
	if (Psi1[i] < *vec_it) {
	  currentNum1A += numerator1A[i];
	  currentNum2A += numerator2A[i];
	  if (present[i]) {
	    currentNum1B += numerator1A[i];
	    currentNum2B += numerator2A[i];
	  }

	  if ( i == Psi1.size()-1) {
	    energyEN += (pow(abs(currentNum1A),2)*Nmc/(Nmc-1) - currentNum2A)/(det_energy[i] - E0);
	    energyENLargeEps += (pow(abs(currentNum1B),2)*Nmc/(Nmc-1) - currentNum2B)/(det_energy[i] - E0);
	  }
	  else if (!(Psi1[i] == Psi1[i+1])) {
	    energyEN += ( pow(abs(currentNum1A),2)*Nmc/(Nmc-1) - currentNum2A)/(det_energy[i] - E0);
	    energyENLargeEps += ( pow(abs(currentNum1B),2)*Nmc/(Nmc-1) - currentNum2B)/(det_energy[i] - E0);
	    currentNum1A = 0.;
	    currentNum2A = 0.;
	    currentNum1B = 0.;
	    currentNum2B = 0.;
	  }
	  i++;
	}
	else if (*vec_it <Psi1[i] && vec_it != SortedDets.end())
	  vec_it++;
	else if (*vec_it <Psi1[i] && vec_it == SortedDets.end()) {
	  currentNum1A += numerator1A[i];
	  currentNum2A += numerator2A[i];
	  if (present[i]) {
	    currentNum1B += numerator1A[i];
	    currentNum2B += numerator2A[i];
	  }

	  if ( i == Psi1.size()-1) {
	    energyEN += ( pow(abs(currentNum1A),2)*Nmc/(Nmc-1) - currentNum2A)/(det_energy[i] - E0);
	    energyENLargeEps += ( pow(abs(currentNum1B),2)*Nmc/(Nmc-1) - currentNum2B)/(det_energy[i] - E0);
	  }
	  else if (!(Psi1[i] == Psi1[i+1])) {
	    energyEN += ( pow(abs(currentNum1A),2)*Nmc/(Nmc-1) - currentNum2A)/(det_energy[i] - E0);
	    energyENLargeEps += ( pow(abs(currentNum1B),2)*Nmc/(Nmc-1) - currentNum2B)/(det_energy[i] - E0);
	    //energyEN += (currentNum1A*currentNum1A*Nmc/(Nmc-1) - currentNum2A)/(det_energy[i] - E0);
	    //energyENLargeEps += (currentNum1B*currentNum1B*Nmc/(Nmc-1) - currentNum2B)/(det_energy[i] - E0);
	    currentNum1A = 0.;
	    currentNum2A = 0.;
	    currentNum1B = 0.;
	    currentNum2B = 0.;
	  }
	  i++;
	}
	else {
	  if (Psi1[i] == Psi1[i+1])
	    i++;
	  else {
	    vec_it++; i++;
	  }
	}
      }

      totalPT=0; totalPTLargeEps=0;
#pragma omp barrier
#pragma omp critical
      {
	totalPT += energyEN;
	totalPTLargeEps += energyENLargeEps;
      }
#pragma omp barrier

      double finalE = totalPT, finalELargeEps=totalPTLargeEps;
      if (mpigetrank() == 0 && omp_get_thread_num() == 0) {
	AvgenergyEN += -finalE+finalELargeEps+EptLarge; currentIter++;
	std::cout << format("%6i  %14.8f  %s%i %14.8f   %10.2f  %10i")
	  %(currentIter) % (E0-finalE+finalELargeEps+EptLarge) % ("Root") % root % (E0+AvgenergyEN/currentIter) % (getTime()-startofCalc) % AllDistinctSample ;
	cout << endl;
      }
      else if (mpigetrank() != 0 && omp_get_thread_num() == 0) {
	AvgenergyEN += -finalE+finalELargeEps+EptLarge; currentIter++;
	ofs << format("%6i  %14.8f  %s%i %14.8f   %10.2f  %10i")
	  %(currentIter) % (E0-finalE+finalELargeEps+EptLarge) % ("Root") % root % (E0+AvgenergyEN/currentIter) % (getTime()-startofCalc) % AllDistinctSample ;
	ofs << endl;
      }
    }
  }

}

void SHCIbasics::DoPerturbativeStochastic2SingleList(vector<Determinant>& Dets, MatrixXx& ci, double& E0, oneInt& I1, twoInt& I2,
						    twoIntHeatBathSHM& I2HB, vector<int>& irrep, schedule& schd, double coreE, int nelec, int root) {

  boost::mpi::communicator world;
  char file [5000];
  sprintf (file, "output-%d.bkp" , world.rank() );
  //std::ofstream ofs(file);
  std::ofstream ofs;
  if (root == 0)
    ofs.open(file, std::ofstream::out);
  else
    ofs.open(file, std::ofstream::app);

  int norbs = Determinant::norbs;
  std::vector<Determinant> SortedDets = Dets; std::sort(SortedDets.begin(), SortedDets.end());
  int niter = schd.nPTiter;
  //int niter = 1000000;
  //double eps = 0.001;
  int Nsample = schd.SampleN;
  double AvgenergyEN = 0.0;
  double AverageDen = 0.0;
  int currentIter = 0;
  int sampleSize = 0;
  int num_thrds = omp_get_max_threads();

  double cumulative = 0.0;
  for (int i=0; i<ci.rows(); i++)
    cumulative += abs(ci(i,0));

  std::vector<int> alias; std::vector<double> prob;
  SHCIsampledeterminants::setUpAliasMethod(ci, cumulative, alias, prob);
#pragma omp parallel for schedule(dynamic)
  for (int iter=0; iter<niter; iter++) {
    //cout << norbs<<"  "<<nelec<<endl;
    char psiArray[norbs];
    vector<int> psiClosed(nelec,0);
    vector<int> psiOpen(norbs-nelec,0);
    //char psiArray[norbs];
    std::vector<CItype> wts1(Nsample,0.0); std::vector<int> Sample1(Nsample,-1);

    //int Nmc = sample_N2(ci, cumulative, Sample1, wts1);
    int distinctSample = SHCIsampledeterminants::sample_N2_alias(ci, cumulative, Sample1, wts1, alias, prob);
    int Nmc = Nsample;
    double norm = 0.0;

    size_t initSize = 100000;
    std::vector<Determinant> Psi1; std::vector<CItype>  numerator1; std::vector<double> numerator2;
    std::vector<double>  det_energy;
    Psi1.reserve(initSize); numerator1.reserve(initSize); numerator2.reserve(initSize); det_energy.reserve(initSize);
    for (int i=0; i<distinctSample; i++) {
      int I = Sample1[i];
      SHCIgetdeterminants::getDeterminantsStochastic(Dets[I], schd.epsilon2/abs(ci(I,0)), wts1[i], ci(I,0), I1, I2, I2HB, irrep, coreE, E0, Psi1, numerator1, numerator2, det_energy, schd, Nmc, nelec);
    }

    quickSort( &(Psi1[0]), 0, Psi1.size(), &numerator1[0], &numerator2[0], &det_energy);

    CItype currentNum1=0.; double currentNum2=0.;
    vector<Determinant>::iterator vec_it = SortedDets.begin();
    double energyEN = 0.0;

    for (int i=0;i<Psi1.size();) {
      if (Psi1[i] < *vec_it) {
	currentNum1 += numerator1[i];
	currentNum2 += numerator2[i];
	if ( i == Psi1.size()-1)
	  energyEN += (pow(abs(currentNum1),2)*Nmc/(Nmc-1) - currentNum2)/(det_energy[i] - E0);
	else if (!(Psi1[i] == Psi1[i+1])) {
	  energyEN += ( pow(abs(currentNum1),2)*Nmc/(Nmc-1) - currentNum2)/(det_energy[i] - E0);
	  currentNum1 = 0.;
	  currentNum2 = 0.;
	}
	i++;
      }
      else if (*vec_it <Psi1[i] && vec_it != SortedDets.end())
	vec_it++;
      else if (*vec_it <Psi1[i] && vec_it == SortedDets.end()) {
	currentNum1 += numerator1[i];
	currentNum2 += numerator2[i];
	if ( i == Psi1.size()-1)
	  energyEN += (pow(abs(currentNum1),2)*Nmc/(Nmc-1) - currentNum2)/(det_energy[i] - E0);
	//energyEN += (currentNum1*currentNum1*Nmc/(Nmc-1) - currentNum2)/(det_energy[i] - E0);
	else if (!(Psi1[i] == Psi1[i+1])) {
	  energyEN += ( pow(abs(currentNum1),2)*Nmc/(Nmc-1) - currentNum2)/(det_energy[i] - E0);
	  //energyEN += (currentNum1*currentNum1*Nmc/(Nmc-1) - currentNum2)/(det_energy[i] - E0);
	  currentNum1 = 0.;
	  currentNum2 = 0.;
	}
	i++;
      }
      else {
	if (Psi1[i] == Psi1[i+1])
	  i++;
	else {
	  vec_it++; i++;
	}
      }
    }

    sampleSize = distinctSample;

#pragma omp critical
    {
      if (mpigetrank() == 0) {
	AvgenergyEN += energyEN; currentIter++;
	std::cout << format("%6i  %14.8f  %s%i %14.8f   %10.2f  %10i %4i")
	  %(currentIter) % (E0-energyEN) % ("Root") % root % (E0-AvgenergyEN/currentIter) % (getTime()-startofCalc) % sampleSize % (omp_get_thread_num());
	cout << endl;
      }
      else {
	AvgenergyEN += energyEN; currentIter++;
	ofs << format("%6i  %14.8f  %s%i %14.8f   %10.2f  %10i %4i")
	  %(currentIter) % (E0-energyEN) % ("Root") % root % (E0-AvgenergyEN/currentIter) % (getTime()-startofCalc) % sampleSize % (omp_get_thread_num());
	ofs << endl;

      }
    }
  }
  ofs.close();

}

double SHCIbasics::DoPerturbativeDeterministic(vector<Determinant>& Dets, MatrixXx& ci, double& E0, oneInt& I1, twoInt& I2,
					      twoIntHeatBathSHM& I2HB, vector<int>& irrep, schedule& schd, double coreE, int nelec, int root, bool appendPsi1ToPsi0) {

  boost::mpi::communicator world;
  int norbs = Determinant::norbs;
  std::vector<Determinant> SortedDets = Dets; std::sort(SortedDets.begin(), SortedDets.end());

  double energyEN = 0.0;
  int num_thrds = omp_get_max_threads();

  std::vector<StitchDEH> uniqueDEH(num_thrds);
  std::vector<std::vector< std::vector<vector<Determinant> > > > hashedDetBeforeMPI(mpigetsize(), std::vector<std::vector<vector<Determinant> > >(num_thrds));
  std::vector<std::vector< std::vector<vector<Determinant> > > > hashedDetAfterMPI(mpigetsize(), std::vector<std::vector<vector<Determinant> > >(num_thrds));
  std::vector<std::vector< std::vector<vector<CItype> > > > hashedNumBeforeMPI(mpigetsize(), std::vector<std::vector<vector<CItype> > >(num_thrds));
  std::vector<std::vector< std::vector<vector<CItype> > > > hashedNumAfterMPI(mpigetsize(), std::vector<std::vector<vector<CItype> > >(num_thrds));
  std::vector<std::vector< std::vector<vector<double> > > > hashedEnergyBeforeMPI(mpigetsize(), std::vector<std::vector<vector<double> > >(num_thrds));
  std::vector<std::vector< std::vector<vector<double> > > > hashedEnergyAfterMPI(mpigetsize(), std::vector<std::vector<vector<double> > >(num_thrds));
  std::vector<std::vector< std::vector<vector<vector<int> > > > > hashedVarIndicesBeforeMPI(mpigetsize(), std::vector<std::vector<vector<vector<int> > > >(num_thrds));
  std::vector<std::vector< std::vector<vector<vector<int> > > > > hashedVarIndicesAfterMPI(mpigetsize(), std::vector<std::vector<vector<vector<int> > > >(num_thrds));
  std::vector<std::vector< std::vector<vector<vector<size_t> > > > > hashedOrbdiffBeforeMPI(mpigetsize(), std::vector<std::vector<vector<vector<size_t> > > >(num_thrds));
  std::vector<std::vector< std::vector<vector<vector<size_t> > > > > hashedOrbdiffAfterMPI(mpigetsize(), std::vector<std::vector<vector<vector<size_t> > > >(num_thrds));
  double totalPT = 0.0;
  int ntries = 0;

#pragma omp parallel
  {
    if (schd.DoRDM) {
      uniqueDEH[omp_get_thread_num()].extra_info = true;
      for (int i=0; i<Dets.size(); i++) {
	if (i%(omp_get_num_threads()*mpigetsize()) != mpigetrank()*omp_get_num_threads()+omp_get_thread_num()) {continue;}
	SHCIgetdeterminants::getDeterminantsDeterministicPTKeepRefDets(Dets[i], i, abs(schd.epsilon2/ci(i,0)), ci(i,0),
						I1, I2, I2HB, irrep, coreE, E0,
						*uniqueDEH[omp_get_thread_num()].Det,
						*uniqueDEH[omp_get_thread_num()].Num,
						*uniqueDEH[omp_get_thread_num()].Energy,
						*uniqueDEH[omp_get_thread_num()].var_indices_beforeMerge,
						*uniqueDEH[omp_get_thread_num()].orbDifference_beforeMerge,
						schd, nelec);
	if (i%100000 == 0 && omp_get_thread_num()==0 && mpigetrank() == 0) cout <<"# "<<i<<endl;
      }
    }
    else {
      for (int i=0; i<Dets.size(); i++) {
	if (i%(omp_get_num_threads()*mpigetsize()) != mpigetrank()*omp_get_num_threads()+omp_get_thread_num()) {continue;}
	SHCIgetdeterminants::getDeterminantsDeterministicPT(Dets[i], abs(schd.epsilon2/ci(i,0)), ci(i,0), 0.0,
				   I1, I2, I2HB, irrep, coreE, E0,
				   *uniqueDEH[omp_get_thread_num()].Det,
				   *uniqueDEH[omp_get_thread_num()].Num,
				   *uniqueDEH[omp_get_thread_num()].Energy,
				   schd,0, nelec);
	if (i%100000 == 0 && omp_get_thread_num()==0 && mpigetrank() == 0) cout <<"# "<<i<<endl;
      }
    }


    uniqueDEH[omp_get_thread_num()].MergeSortAndRemoveDuplicates();
    uniqueDEH[omp_get_thread_num()].RemoveDetsPresentIn(SortedDets);

    if(mpigetsize() >1 || num_thrds >1) {
      StitchDEH uniqueDEH_afterMPI;
      if (schd.DoRDM) uniqueDEH_afterMPI.extra_info = true;
      if (mpigetrank() == 0 && omp_get_thread_num() == 0) cout << "#Before hash "<<getTime()-startofCalc<<endl;


      for (int proc=0; proc<mpigetsize(); proc++) {
	hashedDetBeforeMPI[proc][omp_get_thread_num()].resize(num_thrds);
	hashedNumBeforeMPI[proc][omp_get_thread_num()].resize(num_thrds);
	hashedEnergyBeforeMPI[proc][omp_get_thread_num()].resize(num_thrds);
	if (schd.DoRDM) {
	  hashedVarIndicesBeforeMPI[proc][omp_get_thread_num()].resize(num_thrds);
	  hashedOrbdiffBeforeMPI[proc][omp_get_thread_num()].resize(num_thrds);
	}
      }

      if (omp_get_thread_num()==0 ) {
	if (mpigetsize() == 1)
	  ntries = 1;
	else {
	  ntries = uniqueDEH[omp_get_thread_num()].Det->size()*DetLen*2*omp_get_num_threads()/268435400+1;
	  mpi::broadcast(world, ntries, 0);
	}
      }
#pragma omp barrier

      size_t batchsize = uniqueDEH[omp_get_thread_num()].Det->size()/ntries;
      //ntries = 1;
      for (int tries = 0; tries<ntries; tries++) {

        size_t start = (ntries-1-tries)*batchsize;
        size_t end   = tries==0 ? uniqueDEH[omp_get_thread_num()].Det->size() : (ntries-tries)*batchsize;

	for (size_t j=start; j<end; j++) {
	  size_t lOrder = uniqueDEH[omp_get_thread_num()].Det->at(j).getHash();
	  size_t procThrd = lOrder%(mpigetsize()*num_thrds);
	  int proc = abs(procThrd/num_thrds), thrd = abs(procThrd%num_thrds);
	  hashedDetBeforeMPI[proc][omp_get_thread_num()][thrd].push_back(uniqueDEH[omp_get_thread_num()].Det->at(j));
	  hashedNumBeforeMPI[proc][omp_get_thread_num()][thrd].push_back(uniqueDEH[omp_get_thread_num()].Num->at(j));
	  hashedEnergyBeforeMPI[proc][omp_get_thread_num()][thrd].push_back(uniqueDEH[omp_get_thread_num()].Energy->at(j));
	  if (schd.DoRDM) {
	    hashedVarIndicesBeforeMPI[proc][omp_get_thread_num()][thrd].push_back(uniqueDEH[omp_get_thread_num()].var_indices->at(j));
	    hashedOrbdiffBeforeMPI[proc][omp_get_thread_num()][thrd].push_back(uniqueDEH[omp_get_thread_num()].orbDifference->at(j));
	  }
	}

	uniqueDEH[omp_get_thread_num()].resize(start);

	if (mpigetrank() == 0 && omp_get_thread_num() == 0) cout << "#After hash "<<getTime()-startofCalc<<endl;

#pragma omp barrier
	if (omp_get_thread_num()==num_thrds-1) {
	  mpi::all_to_all(world, hashedDetBeforeMPI, hashedDetAfterMPI);
	  mpi::all_to_all(world, hashedNumBeforeMPI, hashedNumAfterMPI);
	  mpi::all_to_all(world, hashedEnergyBeforeMPI, hashedEnergyAfterMPI);
	  if (schd.DoRDM) {
	    mpi::all_to_all(world, hashedVarIndicesBeforeMPI, hashedVarIndicesAfterMPI);
	    mpi::all_to_all(world, hashedOrbdiffBeforeMPI, hashedOrbdiffAfterMPI);
	  }
	}
#pragma omp barrier

	for (int proc=0; proc<mpigetsize(); proc++) {
	  for (int thrd=0; thrd<num_thrds; thrd++) {
	    hashedDetBeforeMPI[proc][thrd][omp_get_thread_num()].clear();
	    hashedNumBeforeMPI[proc][thrd][omp_get_thread_num()].clear();
	    hashedEnergyBeforeMPI[proc][thrd][omp_get_thread_num()].clear();
	    if (schd.DoRDM) {
	      hashedVarIndicesBeforeMPI[proc][thrd][omp_get_thread_num()].clear();
	      hashedOrbdiffBeforeMPI[proc][thrd][omp_get_thread_num()].clear();
	    }
	  }
	}


	if (mpigetrank() == 0 && omp_get_thread_num() == 0) cout << "#After all_to_all "<<getTime()-startofCalc<<endl;

	for (int proc=0; proc<mpigetsize(); proc++) {
	  for (int thrd=0; thrd<num_thrds; thrd++) {
	    for (int j=0; j<hashedDetAfterMPI[proc][thrd][omp_get_thread_num()].size(); j++) {
	      uniqueDEH_afterMPI.Det->push_back(hashedDetAfterMPI[proc][thrd][omp_get_thread_num()].at(j));
	      uniqueDEH_afterMPI.Num->push_back(hashedNumAfterMPI[proc][thrd][omp_get_thread_num()].at(j));
	      uniqueDEH_afterMPI.Energy->push_back(hashedEnergyAfterMPI[proc][thrd][omp_get_thread_num()].at(j));
	      if (schd.DoRDM) {
		uniqueDEH_afterMPI.var_indices->push_back(hashedVarIndicesAfterMPI[proc][thrd][omp_get_thread_num()].at(j));
		uniqueDEH_afterMPI.orbDifference->push_back(hashedOrbdiffAfterMPI[proc][thrd][omp_get_thread_num()].at(j));
	      }
	    }
	    hashedDetAfterMPI[proc][thrd][omp_get_thread_num()].clear();
	    hashedNumAfterMPI[proc][thrd][omp_get_thread_num()].clear();
	    hashedEnergyAfterMPI[proc][thrd][omp_get_thread_num()].clear();
	    if (schd.DoRDM) {
	      hashedVarIndicesAfterMPI[proc][thrd][omp_get_thread_num()].clear();
	      hashedOrbdiffAfterMPI[proc][thrd][omp_get_thread_num()].clear();
	    }
	  }
	}
      }


      *uniqueDEH[omp_get_thread_num()].Det = *uniqueDEH_afterMPI.Det;
      *uniqueDEH[omp_get_thread_num()].Num = *uniqueDEH_afterMPI.Num;
      *uniqueDEH[omp_get_thread_num()].Energy = *uniqueDEH_afterMPI.Energy;
      if (schd.DoRDM) {
	*uniqueDEH[omp_get_thread_num()].var_indices = *uniqueDEH_afterMPI.var_indices;
	*uniqueDEH[omp_get_thread_num()].orbDifference = *uniqueDEH_afterMPI.orbDifference;
      }
      uniqueDEH_afterMPI.clear();
      uniqueDEH[omp_get_thread_num()].MergeSortAndRemoveDuplicates();


    }
    if (mpigetrank() == 0 && omp_get_thread_num() == 0) cout << "#After collecting "<<getTime()-startofCalc<<endl;

    //uniqueDEH[omp_get_thread_num()].RemoveDetsPresentIn(SortedDets);
    if (mpigetrank() == 0 && omp_get_thread_num() == 0) cout << "#Unique determinants "<<getTime()-startofCalc<<"  "<<endl;


    vector<Determinant>& hasHEDDets = *uniqueDEH[omp_get_thread_num()].Det;
    vector<CItype>& hasHEDNumerator = *uniqueDEH[omp_get_thread_num()].Num;
    vector<double>& hasHEDEnergy = *uniqueDEH[omp_get_thread_num()].Energy;

    double PTEnergy = 0.0;
    for (size_t i=0; i<hasHEDDets.size();i++) {
      PTEnergy += pow(abs(hasHEDNumerator[i]),2)/(E0-hasHEDEnergy[i]);
    }
#pragma omp critical
    {
      totalPT += PTEnergy;
    }

  }


  double finalE = 0.;
  mpi::all_reduce(world, totalPT, finalE, std::plus<double>());

  if (mpigetrank() == 0) cout << "#Done energy "<<E0+finalE<<"  "<<getTime()-startofCalc<<endl;


  if (schd.DoRDM) {

    int nSpatOrbs = norbs/2;

    //Read the variational RDM
    MatrixXx s2RDM, twoRDM;
    if (schd.DoSpinRDM ){
      if (mpigetrank() == 0) {
	char file [5000];
	sprintf (file, "%s/%d-spinRDM.bkp" , schd.prefix[0].c_str(), root );
	std::ifstream ifs(file, std::ios::binary);
	boost::archive::binary_iarchive load(ifs);
	load >> twoRDM;
	ComputeEnergyFromSpinRDM(norbs, nelec, I1, I2, coreE, twoRDM);
      }
      else
	twoRDM.setZero(norbs*(norbs+1)/2, norbs*(norbs+1)/2);
    }
    if (mpigetrank() == 0) {
      char file [5000];
      sprintf (file, "%s/%d-spatialRDM.bkp" , schd.prefix[0].c_str(), root );
      std::ifstream ifs(file, std::ios::binary);
      boost::archive::binary_iarchive load(ifs);
      load >> s2RDM;
      ComputeEnergyFromSpatialRDM(nSpatOrbs, nelec, I1, I2, coreE, s2RDM);
    }
    else
      s2RDM.setZero(nSpatOrbs*nSpatOrbs, nSpatOrbs*nSpatOrbs);


    for (int thrd = 0; thrd <num_thrds; thrd++)
      {

	vector<Determinant>& uniqueDets = *uniqueDEH[thrd].Det;
	vector<double>& uniqueEnergy = *uniqueDEH[thrd].Energy;
	vector<CItype>& uniqueNumerator = *uniqueDEH[thrd].Num;
	vector<vector<int> >& uniqueVarIndices = *uniqueDEH[thrd].var_indices;
	vector<vector<size_t> >& uniqueOrbDiff = *uniqueDEH[thrd].orbDifference;

	for (size_t k=0; k<uniqueDets.size();k++) {
	  for (size_t i=0; i<uniqueVarIndices[k].size(); i++){
	    int d0=uniqueOrbDiff[k][i]%norbs, c0=(uniqueOrbDiff[k][i]/norbs)%norbs;

	    if (uniqueOrbDiff[k][i]/norbs/norbs == 0) { // single excitation
	      vector<int> closed(nelec, 0);
	      vector<int> open(norbs-nelec,0);
	      Dets[uniqueVarIndices[k][i]].getOpenClosed(open, closed);
	      for (int n1=0;n1<nelec; n1++) {
		double sgn = 1.0;
		int a=max(closed[n1],c0), b=min(closed[n1],c0), I=max(closed[n1],d0), J=min(closed[n1],d0);
		if (closed[n1] == d0) continue;
		Dets[uniqueVarIndices[k][i]].parity(min(d0,c0), max(d0,c0),sgn);
		if (!( (closed[n1] > c0 && closed[n1] > d0) || (closed[n1] < c0 && closed[n1] < d0))) sgn *=-1.;
		if (schd.DoSpinRDM) {
		  twoRDM(a*(a+1)/2+b, I*(I+1)/2+J) += 0.5*sgn*uniqueNumerator[k]*ci(uniqueVarIndices[k][i],0)/(E0-uniqueEnergy[k]);
		  twoRDM(I*(I+1)/2+J, a*(a+1)/2+b) += 0.5*sgn*uniqueNumerator[k]*ci(uniqueVarIndices[k][i],0)/(E0-uniqueEnergy[k]);
		}
		populateSpatialRDM(a, b, I, J, s2RDM, 0.5*sgn*uniqueNumerator[k]*ci(uniqueVarIndices[k][i],0)/(E0-uniqueEnergy[k]), nSpatOrbs);
		populateSpatialRDM(I, J, a, b, s2RDM, 0.5*sgn*uniqueNumerator[k]*ci(uniqueVarIndices[k][i],0)/(E0-uniqueEnergy[k]), nSpatOrbs);
	      } // for n1
	    }  // single
	    else { // double excitation
	      int d1=(uniqueOrbDiff[k][i]/norbs/norbs)%norbs, c1=(uniqueOrbDiff[k][i]/norbs/norbs/norbs)%norbs ;
	      double sgn = 1.0;
	      Dets[uniqueVarIndices[k][i]].parity(d1,d0,c1,c0,sgn);
	      int P = max(c1,c0), Q = min(c1,c0), R = max(d1,d0), S = min(d1,d0);
	      if (P != c0)  sgn *= -1;
	      if (Q != d0)  sgn *= -1;

	      if (schd.DoSpinRDM) {
		twoRDM(P*(P+1)/2+Q, R*(R+1)/2+S) += 0.5*sgn*uniqueNumerator[k]*ci(uniqueVarIndices[k][i],0)/(E0-uniqueEnergy[k]);
		twoRDM(R*(R+1)/2+S, P*(P+1)/2+Q) += 0.5*sgn*uniqueNumerator[k]*ci(uniqueVarIndices[k][i],0)/(E0-uniqueEnergy[k]);
	      }

	      populateSpatialRDM(P, Q, R, S, s2RDM, 0.5*sgn*uniqueNumerator[k]*ci(uniqueVarIndices[k][i],0)/(E0-uniqueEnergy[k]), nSpatOrbs);
	      populateSpatialRDM(R, S, P, Q, s2RDM, 0.5*sgn*uniqueNumerator[k]*ci(uniqueVarIndices[k][i],0)/(E0-uniqueEnergy[k]), nSpatOrbs);
	    }// If
	  } // i in variational connections to PT det k
	} // k in PT dets
      } //thrd in num_thrds

    if (schd.DoSpinRDM)
      MPI_Allreduce(MPI_IN_PLACE, &twoRDM(0,0), twoRDM.rows()*twoRDM.cols(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(MPI_IN_PLACE, &s2RDM(0,0), s2RDM.rows()*s2RDM.cols(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);


    if(mpigetrank() == 0) {
      char file [5000];
      sprintf (file, "%s/spatialRDM.%d.%d.txt" , schd.prefix[0].c_str(), root, root );
      std::ofstream ofs(file, std::ios::out);
      ofs << nSpatOrbs<<endl;

      for (int n1=0; n1<nSpatOrbs; n1++)
	for (int n2=0; n2<nSpatOrbs; n2++)
	  for (int n3=0; n3<nSpatOrbs; n3++)
	    for (int n4=0; n4<nSpatOrbs; n4++)
	      {
		if (abs(s2RDM(n1*nSpatOrbs+n2, n3*nSpatOrbs+n4))  > 1.e-6)
		  ofs << str(boost::format("%3d   %3d   %3d   %3d   %10.8g\n") % n1 % n2 % n3 % n4 % s2RDM(n1*nSpatOrbs+n2, n3*nSpatOrbs+n4));
	      }
      ofs.close();


      if (schd.DoSpinRDM) {
	char file [5000];
	sprintf (file, "%s/%d-spinRDM.bkp" , schd.prefix[0].c_str(), root );
	std::ofstream ofs(file, std::ios::binary);
	boost::archive::binary_oarchive save(ofs);
	save << twoRDM;
	ComputeEnergyFromSpinRDM(norbs, nelec, I1, I2, coreE, twoRDM);
      }

      {
	char file [5000];
	sprintf (file, "%s/%d-spatialRDM.bkp" , schd.prefix[0].c_str(), root );
	std::ofstream ofs(file, std::ios::binary);
	boost::archive::binary_oarchive save(ofs);
	save << s2RDM;
	ComputeEnergyFromSpatialRDM(nSpatOrbs, nelec, I1, I2, coreE, s2RDM);
      }
    }
  }//schd.DoRDM


  /*
    if (0) {
    //Calculate the third order energy
    //this only works for one mpi process calculations
    size_t orbDiff;
    double norm = 0.0;
    double diag1 = 0.0; //<psi1|H0|psi1>
    #pragma omp parallel for reduction(+:norm, PT3,diag1)
    for (int i=0; i<uniqueDets.size(); i++) {
    //cout << uniqueNumerator[i]/(E0-uniqueEnergy[i])<<"  "<<(*uniqueDEH[0].Det)[i]<<endl;
    norm += pow(abs(uniqueNumerator[i]),2)/(E0-uniqueEnergy[i])/(E0-uniqueEnergy[i]);
    diag1 += uniqueEnergy[i]*pow(abs(uniqueNumerator[i])/(E0-uniqueEnergy[i]), 2);
    for (int j=i+1; j<uniqueDets.size(); j++)
    if (uniqueDets[i].connected(uniqueDets[j])) {
    CItype hij = Hij(uniqueDets[i], uniqueDets[j], I1, I2, coreE, orbDiff);
    PT3 += 2.*pow(abs(hij*uniqueNumerator[i]),2)/(E0-uniqueEnergy[i])/(E0-uniqueEnergy[j]);
    }
    }
    cout << "PT3 "<<PT3<<"  "<<norm<<"  "<<PT3+E0+finalE<<"  "<<(E0+2*finalE+PT3+diag1)/(1+norm)<<endl;
    PT3 = 0;
    }

    //Calculate Psi1 and then add it to Psi10
    if (appendPsi1ToPsi0) {
    #ifndef SERIAL
    for (int level = 0; level <ceil(log2(mpigetsize())); level++) {

    if (mpigetrank()%ipow(2, level+1) == 0 && mpigetrank() + ipow(2, level) < mpigetsize()) {
    StitchDEH recvDEH;
    int getproc = mpigetrank()+ipow(2,level);
    world.recv(getproc, mpigetsize()*level+getproc, recvDEH);
    uniqueDEH[0].merge(recvDEH);
    uniqueDEH[0].RemoveDuplicates();
    }
    else if ( mpigetrank()%ipow(2, level+1) == 0 && mpigetrank() + ipow(2, level) >= mpigetsize()) {
    continue ;
    }
    else if ( mpigetrank()%ipow(2, level) == 0) {
    int toproc = mpigetrank()-ipow(2,level);
    world.send(toproc, mpigetsize()*level+mpigetrank(), uniqueDEH[0]);
    }
    }


    mpi::broadcast(world, uniqueDEH[0], 0);
    #endif
    vector<Determinant>& newDets = *uniqueDEH[0].Det;
    ci.conservativeResize(ci.rows()+newDets.size(), 1);

    vector<Determinant>::iterator vec_it = Dets.begin();
    int ciindex = 0, initialSize = Dets.size();
    double EPTguess = 0.0;
    for (vector<Determinant>::iterator it=newDets.begin(); it!=newDets.end(); ) {
    if (schd.excitation != 1000 ) {
    if (it->ExcitationDistance(Dets[0]) > schd.excitation) continue;
    }
    Dets.push_back(*it);
    ci(initialSize+ciindex,0) = uniqueDEH[0].Num->at(ciindex)/(E0 - uniqueDEH[0].Energy->at(ciindex));
    ciindex++; it++;
    }

    }

    uniqueDEH[0].clear();
  */
  return finalE;
}


void SHCIbasics::DoPerturbativeDeterministicOffdiagonal(vector<Determinant>& Dets, MatrixXx& ci1, double& E01, 
							MatrixXx&ci2, double& E02, oneInt& I1, twoInt& I2,
							twoIntHeatBathSHM& I2HB, vector<int>& irrep, 
							schedule& schd, double coreE, int nelec, int root, 
							CItype& EPT1, CItype& EPT2, CItype& EPT12) {

  boost::mpi::communicator world;
  int norbs = Determinant::norbs;
  std::vector<Determinant> SortedDets = Dets; std::sort(SortedDets.begin(), SortedDets.end());

  double energyEN = 0.0;
  int num_thrds = omp_get_max_threads();

  std::vector<StitchDEH> uniqueDEH(num_thrds);
  std::vector<std::vector< std::vector<vector<Determinant> > > > hashedDetBeforeMPI(mpigetsize(), std::vector<std::vector<vector<Determinant> > >(num_thrds));
  std::vector<std::vector< std::vector<vector<Determinant> > > > hashedDetAfterMPI(mpigetsize(), std::vector<std::vector<vector<Determinant> > >(num_thrds));
  std::vector<std::vector< std::vector<vector<CItype> > > > hashedNumBeforeMPI(mpigetsize(), std::vector<std::vector<vector<CItype> > >(num_thrds));
  std::vector<std::vector< std::vector<vector<CItype> > > > hashedNumAfterMPI(mpigetsize(), std::vector<std::vector<vector<CItype> > >(num_thrds));
  std::vector<std::vector< std::vector<vector<CItype> > > > hashedNum2BeforeMPI(mpigetsize(), std::vector<std::vector<vector<CItype> > >(num_thrds));
  std::vector<std::vector< std::vector<vector<CItype> > > > hashedNum2AfterMPI(mpigetsize(), std::vector<std::vector<vector<CItype> > >(num_thrds));
  std::vector<std::vector< std::vector<vector<double> > > > hashedEnergyBeforeMPI(mpigetsize(), std::vector<std::vector<vector<double> > >(num_thrds));
  std::vector<std::vector< std::vector<vector<double> > > > hashedEnergyAfterMPI(mpigetsize(), std::vector<std::vector<vector<double> > >(num_thrds));
  CItype totalPT1 = 0.0, totalPT2=0., totalPT12=0.;
  int ntries = 0;

#pragma omp parallel
  {

    for (int i=0; i<Dets.size(); i++) {
      if (i%(omp_get_num_threads()*mpigetsize()) != mpigetrank()*omp_get_num_threads()+omp_get_thread_num()) {continue;}
      SHCIgetdeterminants::getDeterminantsDeterministicPTWithSOC(Dets[i], i, abs(schd.epsilon2/ci1(i,0)), ci1(i,0),
						      abs(schd.epsilon2/ci2(i,0)), ci2(i,0),
						      I1, I2, I2HB, irrep, coreE,
						      *uniqueDEH[omp_get_thread_num()].Det,
						      *uniqueDEH[omp_get_thread_num()].Num,
						      *uniqueDEH[omp_get_thread_num()].Num2,
						      *uniqueDEH[omp_get_thread_num()].Energy,
						      schd, nelec);
    }
    
    uniqueDEH[omp_get_thread_num()].MergeSortAndRemoveDuplicates();
    uniqueDEH[omp_get_thread_num()].RemoveDetsPresentIn(SortedDets);

    if(mpigetsize() >1 || num_thrds >1) {
      StitchDEH uniqueDEH_afterMPI;
      if (schd.DoRDM) uniqueDEH_afterMPI.extra_info = true;
      //if (mpigetrank() == 0 && omp_get_thread_num() == 0) cout << "#Before hash "<<getTime()-startofCalc<<endl;


      for (int proc=0; proc<mpigetsize(); proc++) {
	hashedDetBeforeMPI[proc][omp_get_thread_num()].resize(num_thrds);
	hashedNumBeforeMPI[proc][omp_get_thread_num()].resize(num_thrds);
	hashedNum2BeforeMPI[proc][omp_get_thread_num()].resize(num_thrds);
	hashedEnergyBeforeMPI[proc][omp_get_thread_num()].resize(num_thrds);
      }

      if (omp_get_thread_num()==0 ) {
	ntries = uniqueDEH[omp_get_thread_num()].Det->size()*DetLen*2*omp_get_num_threads()/268435400+1;
	mpi::broadcast(world, ntries, 0);
      }
#pragma omp barrier

      size_t batchsize = uniqueDEH[omp_get_thread_num()].Det->size()/ntries;
      //ntries = 1;
      for (int tries = 0; tries<ntries; tries++) {

        size_t start = (ntries-1-tries)*batchsize;
        size_t end   = tries==0 ? uniqueDEH[omp_get_thread_num()].Det->size() : (ntries-tries)*batchsize;

	for (size_t j=start; j<end; j++) {
	  size_t lOrder = uniqueDEH[omp_get_thread_num()].Det->at(j).getHash();
	  size_t procThrd = lOrder%(mpigetsize()*num_thrds);
	  int proc = abs(procThrd/num_thrds), thrd = abs(procThrd%num_thrds);
	  hashedDetBeforeMPI[proc][omp_get_thread_num()][thrd].push_back(uniqueDEH[omp_get_thread_num()].Det->at(j));
	  hashedNumBeforeMPI[proc][omp_get_thread_num()][thrd].push_back(uniqueDEH[omp_get_thread_num()].Num->at(j));
	  hashedNum2BeforeMPI[proc][omp_get_thread_num()][thrd].push_back(uniqueDEH[omp_get_thread_num()].Num2->at(j));
	  hashedEnergyBeforeMPI[proc][omp_get_thread_num()][thrd].push_back(uniqueDEH[omp_get_thread_num()].Energy->at(j));
	}

	uniqueDEH[omp_get_thread_num()].resize(start);

	//if (mpigetrank() == 0 && omp_get_thread_num() == 0) cout << "#After hash "<<getTime()-startofCalc<<endl;

#pragma omp barrier
	if (omp_get_thread_num()==num_thrds-1) {
	  mpi::all_to_all(world, hashedDetBeforeMPI, hashedDetAfterMPI);
	  mpi::all_to_all(world, hashedNumBeforeMPI, hashedNumAfterMPI);
	  mpi::all_to_all(world, hashedNum2BeforeMPI, hashedNum2AfterMPI);
	  mpi::all_to_all(world, hashedEnergyBeforeMPI, hashedEnergyAfterMPI);
	}
#pragma omp barrier

	for (int proc=0; proc<mpigetsize(); proc++) {
	  for (int thrd=0; thrd<num_thrds; thrd++) {
	    hashedDetBeforeMPI[proc][thrd][omp_get_thread_num()].clear();
	    hashedNumBeforeMPI[proc][thrd][omp_get_thread_num()].clear();
	    hashedNum2BeforeMPI[proc][thrd][omp_get_thread_num()].clear();
	    hashedEnergyBeforeMPI[proc][thrd][omp_get_thread_num()].clear();
	  }
	}


	//if (mpigetrank() == 0 && omp_get_thread_num() == 0) cout << "#After all_to_all "<<getTime()-startofCalc<<endl;

	for (int proc=0; proc<mpigetsize(); proc++) {
	  for (int thrd=0; thrd<num_thrds; thrd++) {
	    for (int j=0; j<hashedDetAfterMPI[proc][thrd][omp_get_thread_num()].size(); j++) {
	      uniqueDEH_afterMPI.Det->push_back(hashedDetAfterMPI[proc][thrd][omp_get_thread_num()].at(j));
	      uniqueDEH_afterMPI.Num->push_back(hashedNumAfterMPI[proc][thrd][omp_get_thread_num()].at(j));
	      uniqueDEH_afterMPI.Num2->push_back(hashedNum2AfterMPI[proc][thrd][omp_get_thread_num()].at(j));
	      uniqueDEH_afterMPI.Energy->push_back(hashedEnergyAfterMPI[proc][thrd][omp_get_thread_num()].at(j));
	    }
	    hashedDetAfterMPI[proc][thrd][omp_get_thread_num()].clear();
	    hashedNumAfterMPI[proc][thrd][omp_get_thread_num()].clear();
	    hashedNum2AfterMPI[proc][thrd][omp_get_thread_num()].clear();
	    hashedEnergyAfterMPI[proc][thrd][omp_get_thread_num()].clear();
	  }
	}
      }


      *uniqueDEH[omp_get_thread_num()].Det = *uniqueDEH_afterMPI.Det;
      *uniqueDEH[omp_get_thread_num()].Num = *uniqueDEH_afterMPI.Num;
      *uniqueDEH[omp_get_thread_num()].Num2 = *uniqueDEH_afterMPI.Num2;
      *uniqueDEH[omp_get_thread_num()].Energy = *uniqueDEH_afterMPI.Energy;
      uniqueDEH_afterMPI.clear();
      uniqueDEH[omp_get_thread_num()].MergeSortAndRemoveDuplicates();


    }

    vector<Determinant>& hasHEDDets = *uniqueDEH[omp_get_thread_num()].Det;
    vector<CItype>& hasHEDNumerator = *uniqueDEH[omp_get_thread_num()].Num;
    vector<CItype>& hasHEDNumerator2 = *uniqueDEH[omp_get_thread_num()].Num2;
    vector<double>& hasHEDEnergy = *uniqueDEH[omp_get_thread_num()].Energy;

    CItype PTEnergy1 = 0.0, PTEnergy2 = 0.0, PTEnergy12 = 0.0;

    for (size_t i=0; i<hasHEDDets.size();i++) {
      PTEnergy1 += pow(abs(hasHEDNumerator[i]),2)/(E01-hasHEDEnergy[i]);
      PTEnergy12 += 0.5*(conj(hasHEDNumerator2[i])*hasHEDNumerator[i]/(E01-hasHEDEnergy[i]) + conj(hasHEDNumerator[i])*hasHEDNumerator2[i]/(E02-hasHEDEnergy[i]));
      PTEnergy2 += pow(abs(hasHEDNumerator2[i]),2)/(E02-hasHEDEnergy[i]);
    }
#pragma omp critical
    {
      totalPT1 += PTEnergy1;
      totalPT12 += PTEnergy12;
      totalPT2 += PTEnergy2;
    }

  }

  EPT1=0.0;EPT2=0.0;EPT12=0.0;
  mpi::all_reduce(world, totalPT1, EPT1, std::plus<CItype>());
  mpi::all_reduce(world, totalPT2, EPT2, std::plus<CItype>());
  mpi::all_reduce(world, totalPT12, EPT12, std::plus<CItype>());

  //if (mpigetrank() == 0) cout << "#Done energy "<<E0+finalE<<"  "<<getTime()-startofCalc<<endl;

}

void SHCIbasics::regenerateH(std::vector<Determinant>& Dets,
				 std::vector<std::vector<int> >&connections,
				 std::vector<std::vector<CItype> >& Helements,
				 oneInt& I1,
				 twoInt& I2,
				 double& coreE
			    ) {

#ifndef SERIAL
  boost::mpi::communicator world;
#endif

#pragma omp parallel
  {
    for (int i=0; i<connections.size(); i++) {
      if ((i/omp_get_num_threads())%world.size() != world.rank()) continue;
      Helements[i][0] = Dets[i].Energy(I1, I2, coreE);
      for (int j=1; j<connections[i].size(); j++) {
	size_t orbDiff;
	Helements[i][j] = Hij(Dets[i], Dets[connections[i][j]], I1, I2, coreE, orbDiff);
      }
    }
  }
}

void SHCIbasics::MakeHfromHelpers(std::map<HalfDet, std::vector<int> >& BetaN,
				 std::map<HalfDet, std::vector<int> >& AlphaNm1,
				 std::vector<Determinant>& Dets,
				 int StartIndex,
				 std::vector<std::vector<int> >&connections,
				 std::vector<std::vector<CItype> >& Helements,
				 int Norbs,
				 oneInt& I1,
				 twoInt& I2,
				 double& coreE,
				 std::vector<std::vector<size_t> >& orbDifference,
				 bool DoRDM) {

#ifndef SERIAL
  boost::mpi::communicator world;
#endif
  int nprocs= mpigetsize(), proc = mpigetrank();

  size_t norbs = Norbs;

#pragma omp parallel
  {
    for (size_t k=StartIndex; k<Dets.size(); k++) {
      if (k%(nprocs*omp_get_num_threads()) != proc*omp_get_num_threads()+omp_get_thread_num()) continue;
      connections[k].push_back(k);
      CItype hij = Dets[k].Energy(I1, I2, coreE);
      Helements[k].push_back(hij);
      if (DoRDM) orbDifference[k].push_back(0);
    }
  }

  std::map<HalfDet, std::vector<int> >::iterator ita = BetaN.begin();
  int index = 0;
  pout <<"# "<< Dets.size()<<"  "<<BetaN.size()<<"  "<<AlphaNm1.size()<<endl;
  pout << "#";
  for (; ita!=BetaN.end(); ita++) {
    std::vector<int>& detIndex = ita->second;
    int localStart = detIndex.size();
    for (int j=0; j<detIndex.size(); j++)
      if (detIndex[j] >= StartIndex) {
	localStart = j; break;
      }

#pragma omp parallel
    {
      for (int k=localStart; k<detIndex.size(); k++) {

	if (detIndex[k]%(nprocs*omp_get_num_threads()) != proc*omp_get_num_threads()+omp_get_thread_num()) continue;

	for(int j=0; j<k; j++) {
	  size_t J = detIndex[j];size_t K = detIndex[k];
	  if (Dets[J].connected(Dets[K])) 	  {
	    connections[K].push_back(J);
	    size_t orbDiff;
	    CItype hij = Hij(Dets[J], Dets[K], I1, I2, coreE, orbDiff);
	    Helements[K].push_back(hij);
	    if (DoRDM)
	      orbDifference[K].push_back(orbDiff);
	  }
	}
      }
    }
    index++;
    if (index%1000000 == 0 && index!= 0) {pout <<". ";}
  }
  pout << format("BetaN    %49.2f\n#")
    % (getTime()-startofCalc);

  ita = AlphaNm1.begin();
  index = 0;
  for (; ita!=AlphaNm1.end(); ita++) {
    std::vector<int>& detIndex = ita->second;
    int localStart = detIndex.size();
    for (int j=0; j<detIndex.size(); j++)
      if (detIndex[j] >= StartIndex) {
	localStart = j; break;
      }

#pragma omp parallel
    {
      for (int k=localStart; k<detIndex.size(); k++) {
	if (detIndex[k]%(nprocs*omp_get_num_threads()) != proc*omp_get_num_threads()+omp_get_thread_num()) continue;

	for(int j=0; j<k; j++) {
	  size_t J = detIndex[j];size_t K = detIndex[k];
	  if (Dets[J].connected(Dets[K]) ) {
	    if (find(connections[K].begin(), connections[K].end(), J) == connections[K].end()){
	      connections[K].push_back(J);
	      size_t orbDiff;
	      CItype hij = Hij(Dets[J], Dets[K], I1, I2, coreE, orbDiff);
	      Helements[K].push_back(hij);

	      if (DoRDM)
		orbDifference[K].push_back(orbDiff);
	    }
	  }
	}
      }
    }
    index++;
    if (index%1000000 == 0 && index!= 0) {pout <<". ";}
  }

  pout << format("AlphaN-1 %49.2f\n")
    % (getTime()-startofCalc);


}

void SHCIbasics::PopulateHelperLists(std::map<HalfDet, std::vector<int> >& BetaN,
				    std::map<HalfDet, std::vector<int> >& AlphaNm1,
				    std::vector<Determinant>& Dets,
				    int StartIndex)
{
  pout << format("#Making Helpers %43.2f\n")
    % (getTime()-startofCalc);
  for (int i=StartIndex; i<Dets.size(); i++) {
    HalfDet da = Dets[i].getAlpha(), db = Dets[i].getBeta();

    BetaN[db].push_back(i);

    int norbs = 64*DetLen;
    std::vector<int> closeda(norbs/2);//, closedb(norbs);
    int ncloseda = da.getClosed(closeda);
    //int nclosedb = db.getClosed(closedb);


    for (int j=0; j<ncloseda; j++) {
      da.setocc(closeda[j], false);
      AlphaNm1[da].push_back(i);
      da.setocc(closeda[j], true);
    }
  }

}

//this takes in a ci vector for determinants placed in Dets
//it then does a SHCI varitional calculation and the resulting
//ci and dets are returned here
//At input usually the Dets will just have a HF or some such determinant
//and ci will be just 1.0
vector<double> SHCIbasics::DoVariational(vector<MatrixXx>& ci, vector<Determinant>& Dets, schedule& schd,
					twoInt& I2, twoIntHeatBathSHM& I2HB, vector<int>& irrep, oneInt& I1, double& coreE
					, int nelec, bool DoRDM) {

  int nroots = ci.size();
  std::map<HalfDet, std::vector<int> > BetaN, AlphaNm1, AlphaN;
  //if I1[1].store.size() is not zero then soc integrals is active so populate AlphaN
  PopulateHelperLists(BetaN, AlphaNm1, Dets, 0);

#ifndef SERIAL
  boost::mpi::communicator world;
#endif
  int num_thrds = omp_get_max_threads();

  std::vector<Determinant> SortedDets = Dets; std::sort(SortedDets.begin(), SortedDets.end());

  size_t norbs = 2.*I2.Direct.rows();
  int Norbs = norbs;
  vector<double> E0(nroots,Dets[0].Energy(I1, I2, coreE));

  pout << "#HF = "<<E0[0]<<std::endl;

  //this is essentially the hamiltonian, we have stored it in a sparse format
  std::vector<std::vector<int> > connections; connections.resize(Dets.size());
  std::vector<std::vector<CItype> > Helements;Helements.resize(Dets.size());
  std::vector<std::vector<size_t> > orbDifference;orbDifference.resize(Dets.size());
  MakeHfromHelpers(BetaN, AlphaNm1, Dets, 0, connections, Helements,
		   norbs, I1, I2, coreE, orbDifference, DoRDM);

#ifdef Complex
  updateSOCconnections(Dets, 0, connections, Helements, norbs, I1, nelec, false);
#endif


  //keep the diagonal energies of determinants so we only have to generated
  //this for the new determinants in each iteration and not all determinants
  MatrixXx diagOld(Dets.size(),1);
  for (int i=0; i<Dets.size(); i++)
    diagOld(i,0) = Dets[i].Energy(I1, I2, coreE);
  int prevSize = 0;

  //If this is a restart calculation then read from disk
  int iterstart = 0;
  if (schd.restart || schd.fullrestart) {
    bool converged;
    readVariationalResult(iterstart, ci, Dets, SortedDets, diagOld, connections, orbDifference, Helements, E0, converged, schd, BetaN, AlphaNm1);
    if (schd.fullrestart)
      iterstart = 0;
    pout << format("# %4i  %10.2e  %10.2e   %14.8f  %10.2f\n")
      %(iterstart) % schd.epsilon1[iterstart] % Dets.size() % E0[0] % (getTime()-startofCalc);
    if (!schd.fullrestart)
      iterstart++;
    else
      regenerateH(Dets, connections, Helements, I1, I2, coreE);
    if (schd.onlyperturbative)
      return E0;

    if (converged && iterstart >= schd.epsilon1.size()) {
      pout << "# restarting from a converged calculation, moving to perturbative part.!!"<<endl;
      return E0;
    }
  }


  //do the variational bit
  for (int iter=iterstart; iter<schd.epsilon1.size(); iter++) {
    double epsilon1 = schd.epsilon1[iter];

    std::vector<StitchDEH> uniqueDEH(num_thrds);

    //for multiple states, use the sum of squares of states to do the seclection process
    pout << format("#-------------Iter=%4i---------------") % iter<<endl;
    MatrixXx cMax(ci[0].rows(),1); cMax = 1.*ci[0];
    for (int j=0; j<ci[0].rows(); j++) {
      cMax(j,0) = pow( abs(cMax(j,0)), 2);
      for (int i=1; i<ci.size(); i++) 
	cMax(j,0) += pow( abs(ci[i](j,0)), 2);
	
      cMax(j,0) = pow( cMax(j,0), 0.5);
    }


    CItype zero = 0.0;

#pragma omp parallel
    {
      for (int i=0; i<SortedDets.size(); i++) {
	if (i%(mpigetsize()*omp_get_num_threads()) != mpigetrank()*omp_get_num_threads()+omp_get_thread_num()) continue;
	SHCIgetdeterminants::getDeterminantsVariational(Dets[i], epsilon1/abs(cMax(i,0)), cMax(i,0), zero,
					       I1, I2, I2HB, irrep, coreE, E0[0],
					       *uniqueDEH[omp_get_thread_num()].Det,
					       schd,0, nelec);
      }
      uniqueDEH[omp_get_thread_num()].Energy->resize(uniqueDEH[omp_get_thread_num()].Det->size(),0.0);
      uniqueDEH[omp_get_thread_num()].Num->resize(uniqueDEH[omp_get_thread_num()].Det->size(),0.0);


      uniqueDEH[omp_get_thread_num()].QuickSortAndRemoveDuplicates();
      uniqueDEH[omp_get_thread_num()].RemoveDetsPresentIn(SortedDets);


#pragma omp barrier
      if (omp_get_thread_num() == 0) {
	for (int thrd=1; thrd<num_thrds; thrd++) {
	  uniqueDEH[0].merge(uniqueDEH[thrd]);
	  uniqueDEH[thrd].clear();
	  uniqueDEH[0].RemoveDuplicates();
	}
      }
    }

    uniqueDEH.resize(1);

#ifndef SERIAL
    for (int level = 0; level <ceil(log2(mpigetsize())); level++) {

      if (mpigetrank()%ipow(2, level+1) == 0 && mpigetrank() + ipow(2, level) < mpigetsize()) {
	StitchDEH recvDEH;
	int getproc = mpigetrank()+ipow(2,level);
	world.recv(getproc, mpigetsize()*level+getproc, recvDEH);
	uniqueDEH[0].merge(recvDEH);
	uniqueDEH[0].RemoveDuplicates();
      }
      else if ( mpigetrank()%ipow(2, level+1) == 0 && mpigetrank() + ipow(2, level) >= mpigetsize()) {
	continue ;
      }
      else if ( mpigetrank()%ipow(2, level) == 0) {
	int toproc = mpigetrank()-ipow(2,level);
	world.send(toproc, mpigetsize()*level+mpigetrank(), uniqueDEH[0]);
      }
    }


    mpi::broadcast(world, uniqueDEH[0], 0);
#endif

    vector<Determinant>& newDets = *uniqueDEH[0].Det;
    //cout << newDets.size()<<"  "<<Dets.size()<<endl;
    vector<MatrixXx> X0(ci.size(), MatrixXx(Dets.size()+newDets.size(), 1));
    for (int i=0; i<ci.size(); i++) {
      X0[i].setZero(Dets.size()+newDets.size(),1);
      X0[i].block(0,0,ci[i].rows(),1) = 1.*ci[i];

    }

    vector<Determinant>::iterator vec_it = SortedDets.begin();
    int ciindex = 0;
    double EPTguess = 0.0;
    for (vector<Determinant>::iterator it=newDets.begin(); it!=newDets.end(); ) {
      if (schd.excitation != 1000 ) {
	if (it->ExcitationDistance(Dets[0]) > schd.excitation) continue;
      }
      Dets.push_back(*it);
      it++;
    }
    uniqueDEH.resize(0);

    if (iter != 0) pout << str(boost::format("#Initial guess(PT) : %18.10g  \n") %(E0[0]+EPTguess) );

    MatrixXx diag(Dets.size(), 1); diag.setZero(diag.size(),1);
    if (mpigetrank() == 0) diag.block(0,0,ci[0].rows(),1)= 1.*diagOld;




    connections.resize(Dets.size());
    Helements.resize(Dets.size());
    orbDifference.resize(Dets.size());



    PopulateHelperLists(BetaN, AlphaNm1, Dets, ci[0].size());
    MakeHfromHelpers(BetaN, AlphaNm1, Dets, SortedDets.size(), connections, Helements,
		     norbs, I1, I2, coreE, orbDifference, DoRDM);
#ifdef Complex
    updateSOCconnections(Dets, SortedDets.size(), connections, Helements, norbs, I1, nelec, false);
#endif

#pragma omp parallel
    {
      for (size_t k=SortedDets.size(); k<Dets.size() ; k++) {
	if (k%(mpigetsize()*omp_get_num_threads()) != mpigetrank()*omp_get_num_threads()+omp_get_thread_num()) continue;
	diag(k,0) = Helements[k].at(0);
      }
    }

#ifndef SERIAL
    MPI_Allreduce(MPI_IN_PLACE, &diag(0,0), diag.rows(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
#endif

    for (size_t i=SortedDets.size(); i<Dets.size(); i++)
      SortedDets.push_back(Dets[i]);
    std::sort(SortedDets.begin(), SortedDets.end());



    double prevE0 = E0[0];
    if (iter == 0) prevE0 = -10.0;
    Hmult2 H(connections, Helements);

    E0 = davidson(H, X0, diag, schd.nroots+10, schd.davidsonTol, false);
    mpi::broadcast(world, E0, 0);
    mpi::broadcast(world, X0, 0);

    for (int i=0; i<E0.size(); i++) {
      ci[i].resize(Dets.size(),1); ci[i] = 1.0*X0[i];
      X0[i].resize(0,0);
    }

    diagOld.resize(Dets.size(),1); diagOld = 1.0*diag;

    if (abs(E0[0]-prevE0) < schd.dE || iter == schd.epsilon1.size()-1)  {

      writeVariationalResult(iter, ci, Dets, SortedDets, diag, connections, orbDifference, Helements, E0, true, schd, BetaN, AlphaNm1);
      if (DoRDM) {
	Helements.resize(0); BetaN.clear(); AlphaNm1.clear();
	for (int i=0; i<schd.nroots; i++) {
	  EvaluateAndStoreRDM(connections, Dets, ci[i], orbDifference, nelec, schd, i);
        } // for i
      }

      break;
    }
    else {
      if (schd.io) writeVariationalResult(iter, ci, Dets, SortedDets, diag, connections,
					  orbDifference, Helements, E0, false, schd, BetaN, AlphaNm1);
    }

    pout << format("###########################################      %10.2f ") %(getTime()-startofCalc)<<endl;
  }
  return E0;

}


void SHCIbasics::writeVariationalResult(int iter, vector<MatrixXx>& ci, vector<Determinant>& Dets, vector<Determinant>& SortedDets,
				       MatrixXx& diag, vector<vector<int> >& connections, vector<vector<size_t> >&orbdifference,
				       vector<vector<CItype> >& Helements,
				       vector<double>& E0, bool converged, schedule& schd,
				       std::map<HalfDet, std::vector<int> >& BetaN,
				       std::map<HalfDet, std::vector<int> >& AlphaNm1) {

#ifndef SERIAL
  boost::mpi::communicator world;
#endif

  /*
    cout << "Variational wavefunction" << endl;
    for (int root=0; root<schd.nroots; root++) {
    pout << "### IMPORTANT DETERMINANTS FOR STATE: "<<root<<endl;
    MatrixXd prevci = 1.*ci[root];
    for (int i=0; i<5; i++) {
    compAbs comp;
    int m = distance(&prevci(0,0), max_element(&prevci(0,0), &prevci(0,0)+prevci.rows(), comp));
    pout <<"#"<< i<<"  "<<prevci(m,0)<<"  "<<Dets[m]<<endl;
    prevci(m,0) = 0.0;
    }
    }
  */

  pout << format("#Begin writing variational wf %29.2f\n")
    % (getTime()-startofCalc);

  char file [5000];
  sprintf (file, "%s/%d-variational.bkp" , schd.prefix[0].c_str(), world.rank() );
  std::ofstream ofs(file, std::ios::binary);
  boost::archive::binary_oarchive save(ofs);
  save << iter <<Dets<<SortedDets;
  int diagrows = diag.rows();
  save << diagrows;
  for (int i=0; i<diag.rows(); i++)
    save << diag(i,0);
  save << ci;
  save << E0;
  save << converged;
  save << connections<<orbdifference<<Helements;
  save << BetaN<< AlphaNm1;
  ofs.close();

  pout << format("#End   writing variational wf %29.2f\n")
    % (getTime()-startofCalc);
}


void SHCIbasics::readVariationalResult(int& iter, vector<MatrixXx>& ci, vector<Determinant>& Dets, vector<Determinant>& SortedDets,
				      MatrixXx& diag, vector<vector<int> >& connections, vector<vector<size_t> >& orbdifference,
				      vector<vector<CItype> >& Helements,
				      vector<double>& E0, bool& converged, schedule& schd,
				      std::map<HalfDet, std::vector<int> >& BetaN,
				      std::map<HalfDet, std::vector<int> >& AlphaNm1) {


#ifndef SERIAL
  boost::mpi::communicator world;
#endif

  pout << format("#Begin reading variational wf %29.2f\n")
    % (getTime()-startofCalc);

  char file [5000];
  sprintf (file, "%s/%d-variational.bkp" , schd.prefix[0].c_str(), world.rank() );
  std::ifstream ifs(file, std::ios::binary);
  boost::archive::binary_iarchive load(ifs);

  load >> iter >> Dets >> SortedDets ;
  int diaglen;
  load >> diaglen;
  ci.resize(1, MatrixXx(diaglen,1)); diag.resize(diaglen,1);
  for (int i=0; i<diag.rows(); i++)
    load >> diag(i,0);

  load >> ci;
  load >> E0;
  if (schd.onlyperturbative) {ifs.close();return;}
  load >> converged;

  load >> connections >> orbdifference >> Helements;
  load >> BetaN>> AlphaNm1;
  ifs.close();

  pout << format("#End   reading variational wf %29.2f\n")
    % (getTime()-startofCalc);
}



void SHCIbasics::updateSOCconnections(vector<Determinant>& Dets, int prevSize, vector<vector<int> >& connections, vector<vector<CItype> >& Helements, int norbs, oneInt& int1, int nelec, bool includeSz) {

  size_t Norbs = norbs;

  map<Determinant, int> SortedDets;
  for (int i=0; i<Dets.size(); i++)
    SortedDets[Dets[i]] = i;


  //#pragma omp parallel for schedule(dynamic)
  for (int x=prevSize; x<Dets.size(); x++) {
    Determinant& d = Dets[x];

    vector<int> closed(nelec,0);
    vector<int> open(norbs-nelec,0);
    d.getOpenClosed(open, closed);
    int nclosed = nelec;
    int nopen = norbs-nclosed;

    //loop over all single excitation and find if they are present in the list
    //on or before the current determinant
    for (int ia=0; ia<nopen*nclosed; ia++){
      int i=ia/nopen, a=ia%nopen;

      CItype integral = int1(open[a],closed[i]);
      if (abs(integral) < 1.e-8) continue;

      Determinant di = d;
      if (open[a]%2 == closed[i]%2 && !includeSz) continue;

      di.setocc(open[a], true); di.setocc(closed[i],false);
      double sgn = 1.0;
      d.parity(min(open[a],closed[i]), max(open[a],closed[i]),sgn);


      map<Determinant, int>::iterator it = SortedDets.find(di);
      if (it != SortedDets.end() ) {
	int y = it->second;
	if (y < x) {
	  connections[x].push_back(y);
	  Helements[x].push_back(integral*sgn);

	}
      }
    }
  }
}
