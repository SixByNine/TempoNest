#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//  Copyright (C) 2013 Lindley Lentati

/*
*    This file is part of TempoNest 
* 
*    TempoNest is free software: you can redistribute it and/or modify 
*    it under the terms of the GNU General Public License as published by 
*    the Free Software Foundation, either version 3 of the License, or 
*    (at your option) any later version. 
*    TempoNest  is distributed in the hope that it will be useful, 
*    but WITHOUT ANY WARRANTY; without even the implied warranty of 
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
*    GNU General Public License for more details. 
*    You should have received a copy of the GNU General Public License 
*    along with TempoNest.  If not, see <http://www.gnu.org/licenses/>. 
*/

/*
*    If you use TempoNest and as a byproduct both Tempo2 and MultiNest
*    then please acknowledge it by citing Lentati L., Alexander P., Hobson M. P. (2013) for TempoNest,
*    Hobbs, Edwards & Manchester (2006) MNRAS, Vol 369, Issue 2, 
*    pp. 655-672 (bibtex: 2006MNRAS.369..655H)
*    or Edwards, Hobbs & Manchester (2006) MNRAS, VOl 372, Issue 4,
*    pp. 1549-1574 (bibtex: 2006MNRAS.372.1549E) when discussing the
*    timing model and MultiNest Papers here.
*/


#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <vector>
#include <gsl/gsl_sf_gamma.h>
#include "dgemm.h"
#include "dgemv.h"
#include "dpotri.h"
#include "dpotrf.h"
#include "dpotrs.h"
#include "tempo2.h"
#include "TempoNest.h"
#include "dgesvd.h"
#include "qrdecomp.h"
#include "cholesky.h"
#include "T2toolkit.h"
#include <fstream>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include <sstream>
#include <iterator>
#include <cstring>

#ifdef HAVE_MLAPACK
#include <mpack/mblas_qd.h>
#include <mpack/mlapack_qd.h>
#include <mpack/mblas_dd.h>
#include <mpack/mlapack_dd.h>
#endif

using namespace std;

void *globalcontext;

void  WriteProfileFreqEvo(std::string longname, int &ndim, int profiledimstart);

void assigncontext(void *context){
        globalcontext=context;
}


int Wrap(int kX, int const kLowerBound, int const kUpperBound)
{
    int range_size = kUpperBound - kLowerBound + 1;

    if (kX < kLowerBound)
        kX += range_size * ((kLowerBound - kX) / range_size + 1);

    return kLowerBound + (kX - kLowerBound) % range_size;
}


void TNothpl(int n,double x,double *pl){


        double a=2.0;
        double b=0.0;
        double c=1.0;
        double y0=1.0;
        double y1=2.0*x;
        pl[0]=1.0;
        pl[1]=2.0*x;


//	printf("I AM IN OTHPL %i \n", n);
        for(int k=2;k<n;k++){

                double c=2.0*(k-1.0);
//		printf("%i %g\n", k, sqrt(double(k*1.0)));
		y0=y0/sqrt(double(k*1.0));
		y1=y1/sqrt(double(k*1.0));
                double yn=(a*x+b)*y1-c*y0;
		yn=yn;///sqrt(double(k));
                pl[k]=yn;///sqrt(double(k));
                y0=y1;
                y1=yn;

        }



}


void TNothplMC(int n,double x,double *pl, int cpos){


        double a=2.0;
        double b=0.0;
        double c=1.0;
        double y0=1.0;
        double y1=2.0*x;
        pl[0+cpos]=1.0;
        pl[1+cpos]=2.0*x;


//	printf("I AM IN OTHPL %i \n", n);
        for(int k=2;k<n;k++){

                double c=2.0*(k-1.0);
//		printf("%i %g\n", k, sqrt(double(k*1.0)));
		y0=y0/sqrt(double(k*1.0));
		y1=y1/sqrt(double(k*1.0));
                double yn=(a*x+b)*y1-c*y0;
		yn=yn;///sqrt(double(k));
                pl[k+cpos]=yn;///sqrt(double(k));
                y0=y1;
                y1=yn;

        }



}

/*
void outputProfile(int ndim){

        int number_of_lines = 0;
        double weightsum=0;

	std::string longname="/home/ltl21/scratch/Pulsars/ProfileData/J1909-10cm/results/AllTOA-40Shape-EFJitter2-J1909-3744-";

        std::ifstream checkfile;
        std::string checkname = longname+".txt";
        checkfile.open(checkname.c_str());
        std::string line;
        while (getline(checkfile, line))
                ++number_of_lines;

        printf("number of lines %i \n",number_of_lines);
        checkfile.close();

        std::ifstream summaryfile;
        std::string fname = longname+".txt";
        summaryfile.open(fname.c_str());



        printf("Writing Profiles \n");

	double *shapecoeff=new double[((MNStruct *)globalcontext)->numshapecoeff];
	int otherdims =  ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps + ((MNStruct *)globalcontext)->numFitEQUAD + ((MNStruct *)globalcontext)->numFitEFAC;

	int Nbins=1000;
	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;
	double **Hermitepoly=new double*[Nbins];
	for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}
	double *shapevec=new double[Nbins];

	std::ofstream profilefile;
	std::string dname = longname+"Profiles.txt";
	
	profilefile.open(dname.c_str());

	double *MeanProfile = new double[Nbins];
	double *ProfileErr = new double[Nbins];

	for(int i=0;i<Nbins;i++){
		MeanProfile[i] = 0;
		ProfileErr[i] = 0;
	}
	

	printf("getting mean \n");
	weightsum=0;
        for(int i=0;i<number_of_lines;i++){


		if(i%100 == 0)printf("line: %i of %i %g \n", i, number_of_lines, double(i)/double(number_of_lines));

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);

		double weight=paramlist[0];

                weightsum += weight;
                double like = paramlist[1];

		int cnum=0;
                for(int i = otherdims; i < otherdims+((MNStruct *)globalcontext)->numshapecoeff; i++){
			shapecoeff[cnum] = paramlist[i+2];
			//printf("coeff %i %g \n", cnum, shapecoeff[cnum]);
			cnum++;
			
                }

		double beta = paramlist[otherdims+((MNStruct *)globalcontext)->numshapecoeff+2];

		for(int j =0; j < Nbins; j++){
	

			double timeinterval = 0.0001;
			double time = -1.0*double(Nbins)*timeinterval/2 + j*timeinterval;

			


			TNothpl(numcoeff,time/beta,Hermitepoly[j]);
			for(int k =0; k < numcoeff; k++){
				double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI)*((MNStruct *)globalcontext)->Factorials[k]);
				Hermitepoly[j][k]=Hermitepoly[j][k]*Bconst*exp(-0.5*time*time/beta/beta);
			}

		}

		shapecoeff[0] = 1.0;
		dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');

		//profilefile << weightsum;
		//profilefile << " ";


		for(int j = 0; j < Nbins; j++){

			//profilefile << shapevec[j];
			//profilefile << " ";
			//printf("shape %i %g \n", j, shapevec[j]);

			MeanProfile[j] = MeanProfile[j] + shapevec[j]*weight;

		}

		//profilefile << "\n";	

	}

	for(int j = 0; j < Nbins; j++){
		MeanProfile[j] = MeanProfile[j]/weightsum;
	}


	summaryfile.close();

	summaryfile.open(fname.c_str());

	printf("Getting Errors \n");


	weightsum=0;
	 for(int i=0;i<number_of_lines;i++){

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);

		double weight=paramlist[0];

                weightsum += weight;
                double like = paramlist[1];

		int cnum=0;
                for(int i = otherdims; i < otherdims+((MNStruct *)globalcontext)->numshapecoeff; i++){
			shapecoeff[cnum] = paramlist[i+2];
			//printf("coeff %i %g \n", cnum, shapecoeff[cnum]);
			cnum++;
			
                }

		double beta = paramlist[otherdims+((MNStruct *)globalcontext)->numshapecoeff+2];

		for(int j =0; j < Nbins; j++){
	

			double timeinterval = 0.0001;
			double time = -1.0*double(Nbins)*timeinterval/2 + j*timeinterval;

			


			TNothpl(numcoeff,time/beta,Hermitepoly[j]);
			for(int k =0; k < numcoeff; k++){
				double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI)*((MNStruct *)globalcontext)->Factorials[k]);
				Hermitepoly[j][k]=Hermitepoly[j][k]*Bconst*exp(-0.5*time*time/beta/beta);
			}

		}

		shapecoeff[0] = 1.0;
		dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');

                for(int j = 0; j < Nbins; j++){
                        ProfileErr[j] += weight*(shapevec[j] - MeanProfile[j])*(shapevec[j] - MeanProfile[j]);
                }
        }


	for(int i = 0; i < Nbins; i++){
		ProfileErr[i] = sqrt(ProfileErr[i]/weightsum);
	}


	for(int i = 0; i < Nbins; i++){
		printf("%i %g %g \n", i, MeanProfile[i], ProfileErr[i]);
	}

	

	//profilefile.close();

}
*/
/*

double  WhiteLogLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){

	clock_t startClock,endClock;

	double *EFAC;
	double *EQUAD;
	int pcount=0;
	
	int numfit=((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps;
	long double LDparams[numfit];
	double Fitparams[numfit];
	double *Resvec=new double[((MNStruct *)globalcontext)->pulse->nobs];

	double NLphase=0;
	//for(int p=0;p<ndim;p++){

	//	Cube[p]=(((MNStruct *)globalcontext)->Dpriors[p][1]-((MNStruct *)globalcontext)->Dpriors[p][0])*Cube[p]+((MNStruct *)globalcontext)->Dpriors[p][0];
	//}

	if(((MNStruct *)globalcontext)->doLinear==0){
	
		for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){
			LDparams[p]=Cube[p]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		}

		NLphase=(double)LDparams[0];
		pcount++;
		for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
			((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];	
			pcount++;
		}
		for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
			((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
			pcount++;
		}
		
//		fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       
		formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,1);       
//		
//		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
//			Resvec[o]=(double)((MNStruct *)globalcontext)->pulse->obsn[o].residual+phase;
//		}
	
	}
	else if(((MNStruct *)globalcontext)->doLinear==1){
	
		for(int p=0;p < numfit; p++){
			Fitparams[p]=Cube[p];
			//printf("FitP: %i %g \n",p,Cube[p]);
			pcount++;
		}
		
		double *Fitvec=new double[((MNStruct *)globalcontext)->pulse->nobs];

		dgemv(((MNStruct *)globalcontext)->DMatrix,Fitparams,Fitvec,((MNStruct *)globalcontext)->pulse->nobs,numfit,'N');
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			Resvec[o]=((MNStruct *)globalcontext)->pulse->obsn[o].residual-Fitvec[o];
			//printf("FitVec: %i %g \n",o,Fitvec[o]);
		}
		
		delete[] Fitvec;
	}

	
	double **Steps;
	if(((MNStruct *)globalcontext)->incStep > 0){
		
		Steps=new double*[((MNStruct *)globalcontext)->incStep];
		
		for(int i = 0; i < ((MNStruct *)globalcontext)->incStep; i++){
			Steps[i]=new double[2];
			Steps[i][0] = Cube[pcount];
			pcount++;
			Steps[i][1] = Cube[pcount];
			pcount++;
		}
	}
		

	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=1;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0, Cube[pcount]);
		}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0, Cube[pcount]);
			pcount++;
		}
	}				

	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,2*Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,2*Cube[pcount]);
			pcount++;
                }
        }

        if(((MNStruct *)globalcontext)->doLinear==0){

                fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       
                formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,1);      
                for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			  //printf("res: %i %g \n", o, (double)((MNStruct *)globalcontext)->pulse->obsn[o].residual);
                          Resvec[o]=(double)((MNStruct *)globalcontext)->pulse->obsn[o].residual+NLphase;
                }
        }


	if(((MNStruct *)globalcontext)->incStep > 0){
		for(int i = 0; i < ((MNStruct *)globalcontext)->incStep; i++){
			double StepAmp = Steps[i][0];
			double StepTime = Steps[i][1];

			for(int o1=0;o1<((MNStruct *)globalcontext)->pulse->nobs; o1++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[o1].bat ;
				if( time > StepTime){
					Resvec[o1] += StepAmp;
				}
			}
		}
	}





	double Chisq=0;
	double noiseval=0;
	double detN=0;
	for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){

                if(((MNStruct *)globalcontext)->numFitEQUAD < 2 && ((MNStruct *)globalcontext)->numFitEFAC < 2){
			if(((MNStruct *)globalcontext)->whitemodel == 0){
				noiseval=pow(((((MNStruct *)globalcontext)->pulse->obsn[o].toaErr)*pow(10.0,-6))*EFAC[0],2) + EQUAD[0];
			}
			else if(((MNStruct *)globalcontext)->whitemodel == 1){
				noiseval= EFAC[0]*EFAC[0]*(pow(((((MNStruct *)globalcontext)->pulse->obsn[o].toaErr)*pow(10.0,-6)),2) + EQUAD[0]);
			}
                }
                else if(((MNStruct *)globalcontext)->numFitEQUAD > 1 || ((MNStruct *)globalcontext)->numFitEFAC > 1){
			if(((MNStruct *)globalcontext)->whitemodel == 0){
                       	 	noiseval=pow(((((MNStruct *)globalcontext)->pulse->obsn[o].toaErr)*pow(10.0,-6))*EFAC[((MNStruct *)globalcontext)->sysFlags[o]],2) + EQUAD[((MNStruct *)globalcontext)->sysFlags[o]];
			}
			else if(((MNStruct *)globalcontext)->whitemodel == 1){
                       	 	noiseval=EFAC[((MNStruct *)globalcontext)->sysFlags[o]]*EFAC[((MNStruct *)globalcontext)->sysFlags[o]]*(pow(((((MNStruct *)globalcontext)->pulse->obsn[o].toaErr)*pow(10.0,-6)),2) + EQUAD[((MNStruct *)globalcontext)->sysFlags[o]]);
			}
                }
	
	

		Chisq += pow(Resvec[o],2)/noiseval;
		detN += log(noiseval);
	}
	
	double lnew = 0;
	if(isnan(detN) || isinf(detN) || isnan(Chisq) || isinf(Chisq)){

		lnew=-pow(10.0,200);
	}
	else{
		lnew = -0.5*(((MNStruct *)globalcontext)->pulse->nobs*log(2*M_PI) + detN + Chisq);	
	}

	delete[] EFAC;
	delete[] Resvec;
	if(((MNStruct *)globalcontext)->incStep > 0){
		for(int i = 0; i < ((MNStruct *)globalcontext)->incStep; i++){
			delete[] Steps[i];
		}
		delete[] Steps;
	}

	return lnew;
}



void LRedLogLike(double *Cube, int &ndim, int &npars, double &lnew, void *globalcontext)
{

	clock_t startClock,endClock;
	double phase=0;
	double *EFAC;
	double *EQUAD;
	int pcount=0;
	int bad=0;
	int numfit=((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps;
	long double LDparams[numfit];
	double Fitparams[numfit];
	double *Resvec=new double[((MNStruct *)globalcontext)->pulse->nobs];

	for(int p=0;p<ndim;p++){

		Cube[p]=(((MNStruct *)globalcontext)->Dpriors[p][1]-((MNStruct *)globalcontext)->Dpriors[p][0])*Cube[p]+((MNStruct *)globalcontext)->Dpriors[p][0];
	}

	if(((MNStruct *)globalcontext)->doLinear==0){
	
		for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){
			LDparams[p]=Cube[p]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		}

		phase=(double)LDparams[0];
		pcount++;
		for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
			((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];	
			pcount++;
		}
		for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
			((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
			pcount++;
		}
		
		if(((MNStruct *)globalcontext)->pulse->param[param_dmmodel].fitFlag[0] == 1){
			int DMnum=((MNStruct *)globalcontext)->pulse[0].dmoffsDMnum;
			for(int i =0; i < DMnum; i++){
				((MNStruct *)globalcontext)->pulse[0].dmoffsDM[i]=Cube[ndim-DMnum+i];
			}
		}
		
		
		fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);      
		formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,1);       
		
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			Resvec[o]=(double)((MNStruct *)globalcontext)->pulse->obsn[o].residual+phase;
			
		}
// 		printf("Phase: %g \n", phase);
	
	}
	else if(((MNStruct *)globalcontext)->doLinear==1){
	
		for(int p=0;p < numfit; p++){
			Fitparams[p]=Cube[p];
			pcount++;
		}
		
		double *Fitvec=new double[((MNStruct *)globalcontext)->pulse->nobs];

		dgemv(((MNStruct *)globalcontext)->DMatrix,Fitparams,Fitvec,((MNStruct *)globalcontext)->pulse->nobs,numfit,'N');
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			Resvec[o]=((MNStruct *)globalcontext)->pulse->obsn[o].residual-Fitvec[o];
		}
		
		delete[] Fitvec;
	}

	if(((MNStruct *)globalcontext)->incStep > 0){
		for(int i = 0; i < ((MNStruct *)globalcontext)->incStep; i++){
			double StepAmp = Cube[pcount];
			pcount++;
			double StepTime = Cube[pcount];
			pcount++;
			for(int o1=0;o1<((MNStruct *)globalcontext)->pulse->nobs; o1++){
				double time = (double)((MNStruct *)globalcontext)->pulse->obsn[o1].bat;
				if(time > StepTime){
					Resvec[o1] += StepAmp;
				}
			}
		}
	}
		

	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=1;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=Cube[pcount];
		}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=Cube[pcount];
			pcount++;
		}
	}				

	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,2*Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,2*Cube[pcount]);
			pcount++;
                }
        }

	int FitRedCoeff=2*(((MNStruct *)globalcontext)->numFitRedCoeff);
	int FitDMCoeff=2*(((MNStruct *)globalcontext)->numFitDMCoeff);
	if(((MNStruct *)globalcontext)->incFloatDM != 0)FitDMCoeff+=2*((MNStruct *)globalcontext)->incFloatDM;
	if(((MNStruct *)globalcontext)->incFloatRed != 0)FitRedCoeff+=2*((MNStruct *)globalcontext)->incFloatRed;
    	int totCoeff=0;
    	if(((MNStruct *)globalcontext)->incRED != 0)totCoeff+=FitRedCoeff;
    	if(((MNStruct *)globalcontext)->incDM != 0)totCoeff+=FitDMCoeff;

	double *WorkNoise=new double[((MNStruct *)globalcontext)->pulse->nobs];
	double *powercoeff=new double[totCoeff];

	double tdet=0;
	double timelike=0;
	double timelike2=0;

	if(((MNStruct *)globalcontext)->whitemodel == 0){
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			
			WorkNoise[o]=pow(((((MNStruct *)globalcontext)->pulse->obsn[o].toaErr)*pow(10.0,-6))*EFAC[((MNStruct *)globalcontext)->sysFlags[o]],2) + EQUAD[((MNStruct *)globalcontext)->sysFlags[o]];
			
			tdet=tdet+log(WorkNoise[o]);
			WorkNoise[o]=1.0/WorkNoise[o];
			timelike=timelike+pow(Resvec[o],2)*WorkNoise[o];
			timelike2=timelike2+pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].residual,2)*WorkNoise[o];

		}
	}
	else if(((MNStruct *)globalcontext)->whitemodel == 1){
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			
			WorkNoise[o]=EFAC[((MNStruct *)globalcontext)->sysFlags[o]]*EFAC[((MNStruct *)globalcontext)->sysFlags[o]]*(pow(((((MNStruct *)globalcontext)->pulse->obsn[o].toaErr)*pow(10.0,-6)),2) + EQUAD[((MNStruct *)globalcontext)->sysFlags[o]]);
			
			tdet=tdet+log(WorkNoise[o]);
			WorkNoise[o]=1.0/WorkNoise[o];
			timelike=timelike+pow(Resvec[o],2)*WorkNoise[o];
			timelike2=timelike2+pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].residual,2)*WorkNoise[o];

		}
	}

	
	double *NFd = new double[totCoeff];
	double **FMatrix=new double*[((MNStruct *)globalcontext)->pulse->nobs];
	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		FMatrix[i]=new double[totCoeff];
	}

	double **NF=new double*[((MNStruct *)globalcontext)->pulse->nobs];
	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		NF[i]=new double[totCoeff];
	}

	double **FNF=new double*[totCoeff];
	for(int i=0;i<totCoeff;i++){
		FNF[i]=new double[totCoeff];
	}





	double start,end;
	int go=0;
	for (int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++)
	  {
	    if (((MNStruct *)globalcontext)->pulse->obsn[i].deleted==0)
	      {
		if (go==0)
		  {
		    go = 1;
		    start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    end  = start;
		  }
		else
		  {
		    if (start > (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    if (end < (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      end = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		  }
	      }
	  }
// 	printf("Total time span = %.6f days = %.6f years\n",end-start,(end-start)/365.25);
	double maxtspan=end-start;

        double *freqs = new double[totCoeff];

        double *DMVec=new double[((MNStruct *)globalcontext)->pulse->nobs];
        double DMKappa = 2.410*pow(10.0,-16);
        int startpos=0;
        double freqdet=0;
        if(((MNStruct *)globalcontext)->incRED==2){
                for (int i=0; i<FitRedCoeff/2; i++){
                        int pnum=pcount;
                        double pc=Cube[pcount];
                        freqs[i]=(double)((MNStruct *)globalcontext)->sampleFreq[i]/maxtspan;
                        freqs[i+FitRedCoeff/2]=freqs[i];

                        powercoeff[i]=pow(10.0,pc)/(maxtspan*24*60*60);///(365.25*24*60*60)/4;
                        powercoeff[i+FitRedCoeff/2]=powercoeff[i];
                        freqdet=freqdet+2*log(powercoeff[i]);
                        pcount++;
                }


                for(int i=0;i<FitRedCoeff/2;i++){
                        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                                double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                                FMatrix[k][i]=cos(2*M_PI*freqs[i]*time);
                        }
                }

                for(int i=0;i<FitRedCoeff/2;i++){
                        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                                double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                                FMatrix[k][i+FitRedCoeff/2]=sin(2*M_PI*freqs[i]*time);
                        }
                }



                startpos=FitRedCoeff;

        }
   else if(((MNStruct *)globalcontext)->incRED==3){

                double redamp=Cube[pcount];
                pcount++;
                double redindex=Cube[pcount];
                pcount++;
// 		printf("red: %g %g \n", redamp, redindex);
                 redamp=pow(10.0, redamp);

                freqdet=0;
                 for (int i=0; i<FitRedCoeff/2; i++){

                        freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[i]/maxtspan;
                        freqs[startpos+i+FitRedCoeff/2]=freqs[startpos+i];

                        powercoeff[i]=redamp*redamp*pow((freqs[i]*365.25),-1.0*redindex)/(maxtspan*24*60*60);///(365.25*24*60*60)/4;
                        powercoeff[i+FitRedCoeff/2]=powercoeff[i];
                        freqdet=freqdet+2*log(powercoeff[i]);


                 }

                for(int i=0;i<FitRedCoeff/2;i++){
                        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                                double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                                FMatrix[k][i]=cos(2*M_PI*freqs[i]*time);
                        }
                }

                for(int i=0;i<FitRedCoeff/2;i++){
                        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                                double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                                FMatrix[k][i+FitRedCoeff/2]=sin(2*M_PI*freqs[i]*time);
                        }
                }


                startpos=FitRedCoeff;

        }


       if(((MNStruct *)globalcontext)->incDM==2){

                for (int i=0; i<FitDMCoeff/2; i++){
                        int pnum=pcount;
                        double pc=Cube[pcount];
                        freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[startpos+i]/maxtspan;
                        freqs[startpos+i+FitDMCoeff/2]=freqs[startpos+i];

                        powercoeff[startpos+i]=pow(10.0,pc)/(maxtspan*24*60*60);///(365.25*24*60*60)/4;
                        powercoeff[startpos+i+FitDMCoeff/2]=powercoeff[startpos+i];
                        freqdet=freqdet+2*log(powercoeff[startpos+i]);
                        pcount++;
                }

                for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                        DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
                }

                for(int i=0;i<FitDMCoeff/2;i++){
                        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                                double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                                FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                        }
                }

                for(int i=0;i<FitDMCoeff/2;i++){
                        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                                double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                                FMatrix[k][startpos+i+FitDMCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                        }
                }



        }
        else if(((MNStruct *)globalcontext)->incDM==3){
                double DMamp=Cube[pcount];
                pcount++;
                double DMindex=Cube[pcount];
                pcount++;

                DMamp=pow(10.0, DMamp);

                 for (int i=0; i<FitDMCoeff/2; i++){
                        freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[startpos+i]/maxtspan;
                        freqs[startpos+i+FitDMCoeff/2]=freqs[startpos+i];

                        powercoeff[startpos+i]=DMamp*DMamp*pow((freqs[startpos+i]*365.25),-1.0*DMindex)/(maxtspan*24*60*60);///(365.25*24*60*60)/4;
                        powercoeff[startpos+i+FitDMCoeff/2]=powercoeff[startpos+i];
                        freqdet=freqdet+2*log(powercoeff[startpos+i]);


                 }
                for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                        DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
                }


                for(int i=0;i<FitDMCoeff/2;i++){
                        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                                double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                                FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                        }
                }

                for(int i=0;i<FitDMCoeff/2;i++){
                        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                                double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                                FMatrix[k][startpos+i+FitDMCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                        }
                }



        }





// 	makeFourier(((MNStruct *)globalcontext)->pulse, FitCoeff, FMatrix);

	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		for(int j=0;j<totCoeff;j++){
// 			printf("%i %i %g %g \n",i,j,WorkNoise[i],FMatrix[i][j]);
			NF[i][j]=WorkNoise[i]*FMatrix[i][j];
		}
	}
	dgemv(NF,Resvec,NFd,((MNStruct *)globalcontext)->pulse->nobs,totCoeff,'T');
	dgemm(FMatrix, NF , FNF, ((MNStruct *)globalcontext)->pulse->nobs, totCoeff, ((MNStruct *)globalcontext)->pulse->nobs, totCoeff, 'T', 'N');


	double **PPFM=new double*[totCoeff];
	for(int i=0;i<totCoeff;i++){
		PPFM[i]=new double[totCoeff];
		for(int j=0;j<totCoeff;j++){
			PPFM[i][j]=0;
		}
	}


	for(int c1=0; c1<totCoeff; c1++){
		PPFM[c1][c1]=1.0/powercoeff[c1];
	}



	for(int j=0;j<totCoeff;j++){
		for(int k=0;k<totCoeff;k++){
			PPFM[j][k]=PPFM[j][k]+FNF[j][k];
		}
	}

        double jointdet=0;
        double freqlike=0;
       double *WorkCoeff = new double[totCoeff];
       for(int o1=0;o1<totCoeff; o1++){
                WorkCoeff[o1]=NFd[o1];
        }


	    int info=0;
        dpotrfInfo(PPFM, totCoeff, jointdet,info);
        dpotrs(PPFM, WorkCoeff, totCoeff);
        for(int j=0;j<totCoeff;j++){
                freqlike += NFd[j]*WorkCoeff[j];
        }
	
	lnew=-0.5*(jointdet+tdet+freqdet+timelike-freqlike);

	if(isnan(lnew) || isinf(lnew)){

		lnew=-pow(10.0,200);
		
	}
	
	//printf("CPULike: %g %g %g %g %g %g \n", lnew, jointdet, tdet, freqdet, timelike, freqlike);
	delete[] DMVec;
	delete[] WorkCoeff;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] WorkNoise;
	delete[] powercoeff;
	delete[] Resvec;
	delete[] NFd;
	delete[] freqs;

	for (int j = 0; j < totCoeff; j++){
		delete[] PPFM[j];
	}
	delete[] PPFM;

	for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
		delete[] NF[j];
	}
	delete[] NF;

	for (int j = 0; j < totCoeff; j++){
		delete[] FNF[j];
	}
	delete[] FNF;

	for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
		delete[] FMatrix[j];
	}
	delete[] FMatrix;

}

*/

double  FastNewLRedMarginLogLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){




	
	printf("In fast like\n");
	int pcount=0;
	
	int numfit=((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps;
	long double LDparams[numfit];
	double Fitparams[numfit];
	double *Resvec=new double[((MNStruct *)globalcontext)->pulse->nobs];
	int fitcount=0;
	
	pcount=0;

	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){
		if(((MNStruct *)globalcontext)->Dpriors[p][1] != ((MNStruct *)globalcontext)->Dpriors[p][0]){

			double val = 0;
			if((((MNStruct *)globalcontext)->LDpriors[p][3]) == 0){
				val = Cube[fitcount];
			}
			if((((MNStruct *)globalcontext)->LDpriors[p][3]) == 1){
				val = pow(10.0,Cube[fitcount]);
			}
			LDparams[p]=val*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
			fitcount++;
		}
		else if(((MNStruct *)globalcontext)->Dpriors[p][1] == ((MNStruct *)globalcontext)->Dpriors[p][0]){
			LDparams[p]=((MNStruct *)globalcontext)->Dpriors[p][0]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		}


	}
	pcount=0;
	double phase=(double)LDparams[0];
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];	
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	
	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,1);       
	
	for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		Resvec[o]=(double)((MNStruct *)globalcontext)->pulse->obsn[o].residual+phase;

	}

	
	pcount=fitcount;

	if(((MNStruct *)globalcontext)->incStep > 0){
		for(int i = 0; i < ((MNStruct *)globalcontext)->incStep; i++){
			double StepAmp = Cube[pcount];
			pcount++;
			double StepTime = Cube[pcount];
			pcount++;
			for(int o1=0;o1<((MNStruct *)globalcontext)->pulse->nobs; o1++){
				if(((MNStruct *)globalcontext)->pulse->obsn[o1].bat > StepTime){
					Resvec[o1] += StepAmp;
				}
			}
		}
	}	


/////////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////////Subtract GLitches/////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	for(int i = 0 ; i < ((MNStruct *)globalcontext)->incGlitch; i++){
		double GlitchMJD = Cube[pcount];
		pcount++;

		double *GlitchAmps = new double[2];
		if(((MNStruct *)globalcontext)->incGlitchTerms==1 || ((MNStruct *)globalcontext)->incGlitchTerms==3){
			GlitchAmps[0]  = Cube[pcount];
			pcount++;
		}
		else if(((MNStruct *)globalcontext)->incGlitchTerms==2){
			GlitchAmps[0]  = Cube[pcount];
                        pcount++;
			GlitchAmps[1]  = Cube[pcount];
                        pcount++;
		}


		for(int o1=0;o1<((MNStruct *)globalcontext)->pulse->nobs; o1++){
                        if(((MNStruct *)globalcontext)->pulse->obsn[o1].bat > GlitchMJD){

				if(((MNStruct *)globalcontext)->incGlitchTerms==1){

					long double arg=0;
					arg=((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0])*86400.0;
					double darg = (double)arg;
                                        Resvec[o1] += GlitchAmps[0]*darg;

				}
				else if(((MNStruct *)globalcontext)->incGlitchTerms==2){
					for(int j = 0; j < ((MNStruct *)globalcontext)->incGlitchTerms; j++){

						long double arg=0;
						if(j==0){
							arg=((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0])*86400.0;
						}
						if(j==1){
							arg=0.5*pow((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)*86400.0,2)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0];
						}
						double darg = (double)arg;
						Resvec[o1] += GlitchAmps[j]*darg;
					}
				}
				else if(((MNStruct *)globalcontext)->incGlitchTerms==3){
					long double arg=0;
					arg=0.5*pow((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)*86400.0,2)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0];
					double darg = (double)arg;
                                        Resvec[o1] += GlitchAmps[0]*darg;
				}


			}
		}
	}
	
/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Get White Noise vector///////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  




	double *Noise;	
	Noise=new double[((MNStruct *)globalcontext)->pulse->nobs];
	
	
	for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		Noise[o] = ((MNStruct *)globalcontext)->PreviousNoise[o];
	}
		
	

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Get Time domain likelihood//////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////// 


/* 
	static unsigned int oldcw;
	if(((MNStruct *)globalcontext)->uselongdouble > 0 ){
		fpu_fix_start(&oldcw);
	}
*/	


	double tdet=0;
	double timelike=0;

	for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		timelike+=Resvec[o]*Resvec[o]*Noise[o];
		tdet -= log(Noise[o]);
	}
/*
	dd_real ddtimelike=0.0;
	if(((MNStruct *)globalcontext)->uselongdouble ==1){
		for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			dd_real res = (dd_real)Resvec[o];
			dd_real Nval = (dd_real)Noise[o];
			dd_real chicomp = res*res*Nval;
			ddtimelike += chicomp;
		}
	}

	
        qd_real qdtimelike=0.0;
        if(((MNStruct *)globalcontext)->uselongdouble ==2){
                for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){

                        qd_real res = (qd_real)Resvec[o];
                        qd_real Nval = (qd_real)Noise[o];
                        qd_real chicomp = res*res*Nval;

                        qdtimelike+=chicomp;
                }
        }
*/

	printf("Fast time like %g %g %i\n", timelike, tdet, ((MNStruct *)globalcontext)->totCoeff);

//////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////Do Algebra/////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////


        int totCoeff=((MNStruct *)globalcontext)->totCoeff;

        int TimetoMargin=0;
        for(int i =0; i < ((MNStruct *)globalcontext)->numFitTiming+((MNStruct *)globalcontext)->numFitJumps; i++){
                if(((MNStruct *)globalcontext)->LDpriors[i][2]==1)TimetoMargin++;
        }
	
	


	int totalsize=totCoeff + TimetoMargin;

	double *NTd = new double[totalsize];

	double **NT=new double*[((MNStruct *)globalcontext)->pulse->nobs];
	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		NT[i]=new double[totalsize];
		for(int j=0;j<totalsize;j++){
                        NT[i][j]=((MNStruct *)globalcontext)->PreviousNT[i][j];
                }

	}

	double **TNT=new double*[totalsize];
	for(int i=0;i<totalsize;i++){
		TNT[i]=new double[totalsize];
		for(int j=0;j<totalsize;j++){
			TNT[i][j] = ((MNStruct *)globalcontext)->PreviousTNT[i][j];
		}
	}
	

	



	dgemv(NT,Resvec,NTd,((MNStruct *)globalcontext)->pulse->nobs,totalsize,'T');



	double freqlike=0;
	double *WorkCoeff = new double[totalsize];
	for(int o1=0;o1<totalsize; o1++){
	    WorkCoeff[o1]=NTd[o1];
	}

	int globalinfo=0;
	int info=0;
	double jointdet = ((MNStruct *)globalcontext)->PreviousJointDet;	
	double freqdet = ((MNStruct *)globalcontext)->PreviousFreqDet;
	double uniformpriorterm = ((MNStruct *)globalcontext)->PreviousUniformPrior;

	info=0;
	dpotrsInfo(TNT, WorkCoeff, totalsize, info);
	if(info != 0)globalinfo=1;


	for(int j=0;j<totalsize;j++){
	    freqlike += NTd[j]*WorkCoeff[j];

	}


	double lnew = -0.5*(tdet+jointdet+freqdet+timelike-freqlike) + uniformpriorterm;
	
	if(isnan(lnew) || isinf(lnew) || globalinfo != 0){

		lnew=-pow(10.0,20);
		
	}

/*
        if(((MNStruct *)globalcontext)->uselongdouble > 0 ){
                fpu_fix_end(&oldcw);
        }
*/


	delete[] WorkCoeff;
	delete[] NTd;
	delete[] Noise;
	delete[] Resvec;
	
	for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
		delete[]NT[j];
	}
	delete[]NT;

	for (int j = 0; j < totalsize; j++){
		delete[]TNT[j];
	}
	delete[]TNT;

	return lnew;


}


void LRedLikeMNWrap(double *Cube, int &ndim, int &npars, double &lnew, void *context){



        for(int p=0;p<ndim;p++){

                Cube[p]=(((MNStruct *)globalcontext)->PriorsArray[p+ndim]-((MNStruct *)globalcontext)->PriorsArray[p])*Cube[p]+((MNStruct *)globalcontext)->PriorsArray[p];
        }


	
	double *DerivedParams = new double[npars];

	double result = NewLRedMarginLogLike(ndim, Cube, npars, DerivedParams, context);


	delete[] DerivedParams;

	lnew = result;

}

double  NewLRedMarginLogLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){

	double uniformpriorterm=0;
	clock_t startClock,endClock;

	double **EFAC;
	double *EQUAD;
	int pcount=0;
	
	int numfit=((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps;
	int TimetoMargin=((MNStruct *)globalcontext)->TimetoMargin;
	long double LDparams[numfit];
	double Fitparams[numfit];
	double *Resvec=new double[((MNStruct *)globalcontext)->pulse->nobs];
	int fitcount=0;
	

	if(((MNStruct *)globalcontext)->doGrades == 1){
		int slowlike = 0;
		double myeps = pow(10.0, -10);
		for(int i = 0; i < ndim; i ++){
			
			int slowindex = ((MNStruct *)globalcontext)->hypercube_indices[i]-1;


			 printf("Cube: %i %i %i %g %g %g \n", i, slowindex, ((MNStruct *)globalcontext)->PolyChordGrades[slowindex], Cube[i], ((MNStruct *)globalcontext)->LastParams[i], fabs(Cube[i] - ((MNStruct *)globalcontext)->LastParams[i]));

			if(((MNStruct *)globalcontext)->PolyChordGrades[slowindex] == 1){

//				printf("Slow Cube: %i %g %g %g \n", i, Cube[slowindex], ((MNStruct *)globalcontext)->LastParams[slowindex], fabs(Cube[slowindex] - ((MNStruct *)globalcontext)->LastParams[slowindex]));

				if(fabs(Cube[slowindex] - ((MNStruct *)globalcontext)->LastParams[slowindex]) > myeps){
					slowlike = 1;
				}
			}

			((MNStruct *)globalcontext)->LastParams[i] = Cube[i];
		}

		if(slowlike == 0){

			double lnew = 0;
			if(((MNStruct *)globalcontext)->PreviousInfo == 0){
				lnew = FastNewLRedMarginLogLike(ndim, Cube, npars, DerivedParams, context);
			}
			else if(((MNStruct *)globalcontext)->PreviousInfo == 1){
				lnew = -pow(10.0,20);
			}

			return lnew;
		}
	}
				

	pcount=0;

	///////////////hacky glitch thing for Vela, comment this out for *anything else*/////////////////////////
	//for(int p=1;p<9; p++){
		//Cube[p] = Cube[0];
	//	printf("Cube 0 %g \n", Cube[0]);
	//}
        //for(int p=10;p<17; p++){
        //      Cube[p] = Cube[9];
	//	printf("Cube 9 %g\n", Cube[9]);
        //}

	///////////////end of hacky glitch thing for Vela///////////////////////////////////////////////////////
	
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){
		if(((MNStruct *)globalcontext)->Dpriors[p][1] != ((MNStruct *)globalcontext)->Dpriors[p][0]){

			double val = 0;
			if((((MNStruct *)globalcontext)->LDpriors[p][3]) == 0){
				val = Cube[fitcount];
			}
			if((((MNStruct *)globalcontext)->LDpriors[p][3]) == 1){
				val = pow(10.0,Cube[fitcount]);
			}
                        if((((MNStruct *)globalcontext)->LDpriors[p][3]) == 2){
                                val = pow(10.0,Cube[fitcount]);
				uniformpriorterm += log(val);
                        }
			LDparams[p]=val*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
			fitcount++;
			//printf("Fit: %i %g %g %.15Lg\n", p, ((MNStruct *)globalcontext)->Dpriors[p][0], ((MNStruct *)globalcontext)->Dpriors[p][1], LDparams[p]);
		}
		else if(((MNStruct *)globalcontext)->Dpriors[p][1] == ((MNStruct *)globalcontext)->Dpriors[p][0]){
			LDparams[p]=((MNStruct *)globalcontext)->Dpriors[p][0]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
			//printf("No Fit: %i %g %g %.15Lg\n", p, ((MNStruct *)globalcontext)->Dpriors[p][0], ((MNStruct *)globalcontext)->Dpriors[p][1], LDparams[p]);
		}


	}
	pcount=0;
	double phase=(double)LDparams[0];
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];	
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}


	if(TimetoMargin != numfit){
		fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       /* Form Barycentric arrival times */
		formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,1);       /* Form residuals */
	}
	
	for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		Resvec[o]=(double)((MNStruct *)globalcontext)->pulse->obsn[o].residual+phase;
		//printf("Res: %i %g %g %g %g %g\n", o, Resvec[o], (double) phase, ((MNStruct *)globalcontext)->Dpriors[0][0], ((MNStruct *)globalcontext)->Dpriors[0][1], (double)((MNStruct *)globalcontext)->LDpriors[0][1] , (double)((MNStruct *)globalcontext)->LDpriors[0][0]);

	}

	
	pcount=fitcount;

	if(((MNStruct *)globalcontext)->incStep > 0){
		for(int i = 0; i < ((MNStruct *)globalcontext)->incStep; i++){


			int GrouptoFit=0;
			double GroupStartTime = 0;

			GrouptoFit = floor(Cube[pcount]);
			pcount++;

			double GLength = ((MNStruct *)globalcontext)->GroupStartTimes[GrouptoFit][1] - ((MNStruct *)globalcontext)->GroupStartTimes[GrouptoFit][0];
                        GroupStartTime = ((MNStruct *)globalcontext)->GroupStartTimes[GrouptoFit][0] + Cube[pcount]*GLength;
	                pcount++;

			double StepAmp = Cube[pcount];
			pcount++;

			//printf("Step details: Group %i S %g F %g SS %g A %g \n", GrouptoFit, ((MNStruct *)globalcontext)->GroupStartTimes[GrouptoFit][0],((MNStruct *)globalcontext)->GroupStartTimes[GrouptoFit][1],GroupStartTime,StepAmp);

			for(int o1=0;o1<((MNStruct *)globalcontext)->pulse->nobs; o1++){
				if(((MNStruct *)globalcontext)->pulse->obsn[o1].sat > GroupStartTime && ((MNStruct *)globalcontext)->GroupNoiseFlags[o1] == GrouptoFit ){
					Resvec[o1] += StepAmp;
				}
			}
		}
	}	


/////////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////////Subtract GLitches/////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	for(int i = 0 ; i < ((MNStruct *)globalcontext)->incGlitch; i++){
		double GlitchMJD = Cube[pcount];
		pcount++;

		double *GlitchAmps = new double[2];
		if(((MNStruct *)globalcontext)->incGlitchTerms==1 || ((MNStruct *)globalcontext)->incGlitchTerms==3){
			GlitchAmps[0]  = Cube[pcount];
			pcount++;
		}
		else if(((MNStruct *)globalcontext)->incGlitchTerms==2){
			GlitchAmps[0]  = Cube[pcount];
                        pcount++;
			GlitchAmps[1]  = Cube[pcount];
                        pcount++;
		}


		for(int o1=0;o1<((MNStruct *)globalcontext)->pulse->nobs; o1++){
                        if(((MNStruct *)globalcontext)->pulse->obsn[o1].bat > GlitchMJD){

				if(((MNStruct *)globalcontext)->incGlitchTerms==1){

					long double arg=0;
					arg=((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0])*86400.0;
					double darg = (double)arg;
                                        Resvec[o1] += GlitchAmps[0]*darg;

				}
				else if(((MNStruct *)globalcontext)->incGlitchTerms==2){
					for(int j = 0; j < ((MNStruct *)globalcontext)->incGlitchTerms; j++){

						long double arg=0;
						if(j==0){
							arg=((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0])*86400.0;
						}
						if(j==1){
							arg=0.5*pow((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)*86400.0,2)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0];
						}
						double darg = (double)arg;
						Resvec[o1] += GlitchAmps[j]*darg;
					}
				}
				else if(((MNStruct *)globalcontext)->incGlitchTerms==3){
					long double arg=0;
					arg=0.5*pow((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)*86400.0,2)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0];
					double darg = (double)arg;
                                        Resvec[o1] += GlitchAmps[0]*darg;
				}


			}
		}
	}
	
/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Get White Noise vector///////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double*[((MNStruct *)globalcontext)->EPolTerms];
		for(int n=1; n <=((MNStruct *)globalcontext)->EPolTerms; n++){
			EFAC[n-1]=new double[((MNStruct *)globalcontext)->systemcount];
			if(n==1){
				for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
					EFAC[n-1][o]=1;
				}
			}
			else{
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                   EFAC[n-1][o]=0;
                }
			}
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double*[((MNStruct *)globalcontext)->EPolTerms];
		for(int n=1; n <=((MNStruct *)globalcontext)->EPolTerms; n++){
			
			EFAC[n-1]=new double[((MNStruct *)globalcontext)->systemcount];
			if(n==1){
				for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
					EFAC[n-1][o]=pow(10.0,Cube[pcount]);
					if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[n-1][o]);}
				}
				pcount++;
			}
			else{
                                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){

                                        EFAC[n-1][o]=pow(10.0,Cube[pcount]);
                                }
                                pcount++;
                        }
		}
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double*[((MNStruct *)globalcontext)->EPolTerms];
		for(int n=1; n <=((MNStruct *)globalcontext)->EPolTerms; n++){
			EFAC[n-1]=new double[((MNStruct *)globalcontext)->systemcount];
			if(n==1){
				for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
					EFAC[n-1][p]=pow(10.0,Cube[pcount]);
					if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[n-1][p]);}
					pcount++;
				}
			}
			else{
                                for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
                                        EFAC[n-1][p]=pow(10.0,Cube[pcount]);
                                        pcount++;
                                }
                        }
		}
	}	

		
	//printf("Equad %i \n", ((MNStruct *)globalcontext)->numFitEQUAD);
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,2*Cube[pcount]);
			if(((MNStruct *)globalcontext)->EQUADPriorType ==1) { uniformpriorterm +=log(pow(10.0,Cube[pcount]));}
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){

			if(((MNStruct *)globalcontext)->includeEQsys[o] == 1){
				//printf("Cube: %i %i %g \n", o, pcount, Cube[pcount]);
				EQUAD[o]=pow(10.0,2*Cube[pcount]);
				if(((MNStruct *)globalcontext)->EQUADPriorType ==1) { uniformpriorterm +=log(pow(10.0,Cube[pcount])); }
				pcount++;
			}
			else{
				EQUAD[o]=0;
			}
			//printf("Equad? %i %g \n", o, EQUAD[o]);
		}
    	}
    

        double *SQUAD;
	if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,2*Cube[pcount]);
			if(((MNStruct *)globalcontext)->EQUADPriorType ==1) { uniformpriorterm +=log(pow(10.0,Cube[pcount]));}
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
        SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
        for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
            SQUAD[o]=pow(10.0,2*Cube[pcount]);
	    if(((MNStruct *)globalcontext)->EQUADPriorType ==1) { uniformpriorterm +=log(pow(10.0,Cube[pcount])); }

			pcount++;
        }
    }
 
	double *ECORRPrior;
	if(((MNStruct *)globalcontext)->incNGJitter >0){
		double *ECorrCoeffs=new double[((MNStruct *)globalcontext)->incNGJitter];	
		for(int i =0; i < ((MNStruct *)globalcontext)->incNGJitter; i++){
			ECorrCoeffs[i] = pow(10.0, 2*Cube[pcount]);
			if(((MNStruct *)globalcontext)->EQUADPriorType ==1) { uniformpriorterm +=log(pow(10.0,Cube[pcount])); }
			pcount++;
		}
    		ECORRPrior = new double[((MNStruct *)globalcontext)->numNGJitterEpochs];
		for(int i =0; i < ((MNStruct *)globalcontext)->numNGJitterEpochs; i++){
			ECORRPrior[i] = ECorrCoeffs[((MNStruct *)globalcontext)->NGJitterSysFlags[i]];
		}

		delete[] ECorrCoeffs;
	} 

	double DMEQUAD = 0;
	if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
		DMEQUAD = pow(10.0, Cube[pcount]);
		pcount++;
	}

	double SolarWind=0;
	double WhiteSolarWind = 0;

	if(((MNStruct *)globalcontext)->FitSolarWind == 1){
		SolarWind = Cube[pcount];
		pcount++;

		//printf("Solar Wind: %g \n", SolarWind);
	}
	if(((MNStruct *)globalcontext)->FitWhiteSolarWind == 1){
		WhiteSolarWind = pow(10.0, Cube[pcount]);
		pcount++;

		//printf("White Solar Wind: %g \n", WhiteSolarWind);
	}



	double *Noise;	
	double *BATvec;
	Noise=new double[((MNStruct *)globalcontext)->pulse->nobs];
	BATvec=new double[((MNStruct *)globalcontext)->pulse->nobs];
	
	
	for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		BATvec[o]=(double)((MNStruct *)globalcontext)->pulse->obsn[o].bat;
	}
		
		

	double DMKappa = 2.410*pow(10.0,-16);
	if(((MNStruct *)globalcontext)->whitemodel == 0){
	
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			double EFACterm=0;
			double noiseval=0;
			double ShannonJitterTerm=0;

			double SWTerm = WhiteSolarWind*((MNStruct *)globalcontext)->pulse->obsn[o].tdis2/((MNStruct *)globalcontext)->pulse->ne_sw;
			double DMEQUADTerm = DMEQUAD/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));	
						
			
			if(((MNStruct *)globalcontext)->useOriginalErrors==0){
				noiseval=((MNStruct *)globalcontext)->pulse->obsn[o].toaErr;
			}
			else if(((MNStruct *)globalcontext)->useOriginalErrors==1){
				noiseval=((MNStruct *)globalcontext)->pulse->obsn[o].origErr;
			}


			for(int n=1; n <=((MNStruct *)globalcontext)->EPolTerms; n++){
				EFACterm=EFACterm + pow((noiseval*pow(10.0,-6))/pow(pow(10.0,-7),n-1),n)*EFAC[n-1][((MNStruct *)globalcontext)->sysFlags[o]];
			}	
			
			if(((MNStruct *)globalcontext)->incShannonJitter > 0){	
			 	ShannonJitterTerm=SQUAD[((MNStruct *)globalcontext)->sysFlags[o]]*((MNStruct *)globalcontext)->TobsInfo[o]/1000.0;
			}
			//printf("Noise: %i %g %g %g %g %g \n", EFACterm, EQUAD[((MNStruct *)globalcontext)->sysFlags[o]], ShannonJitterTerm, SWTerm, DMEQUADTerm);
			Noise[o]= 1.0/(pow(EFACterm,2) + EQUAD[((MNStruct *)globalcontext)->sysFlags[o]]+ShannonJitterTerm + SWTerm*SWTerm + DMEQUADTerm*DMEQUADTerm);

		}
		
	}
	else if(((MNStruct *)globalcontext)->whitemodel == 1){
	
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){

			Noise[o]=1.0/(EFAC[0][((MNStruct *)globalcontext)->sysFlags[o]]*EFAC[0][((MNStruct *)globalcontext)->sysFlags[o]]*(pow(((((MNStruct *)globalcontext)->pulse->obsn[o].toaErr)*pow(10.0,-6)),2) + EQUAD[((MNStruct *)globalcontext)->sysFlags[o]]));
		}
		
	}

	

/////////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////////Initialise TotalMatrix////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////// 

	int totalsize = ((MNStruct *)globalcontext)->totalsize;
	
	double *TotalMatrix=((MNStruct *)globalcontext)->StoredTMatrix;


		
/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form the Design Matrix////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////  


	if(TimetoMargin != ((MNStruct *)globalcontext)->numFitTiming+((MNStruct *)globalcontext)->numFitJumps){

		getCustomDVectorLike(globalcontext, TotalMatrix, ((MNStruct *)globalcontext)->pulse->nobs, TimetoMargin, totalsize);
		vector_dgesvd(TotalMatrix,((MNStruct *)globalcontext)->pulse->nobs, TimetoMargin);

		
	}




//////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////////////Set up Coefficients///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////  


	double maxtspan=((MNStruct *)globalcontext)->Tspan;
	double averageTSamp=2*maxtspan/((MNStruct *)globalcontext)->pulse->nobs;



	int FitRedCoeff=2*(((MNStruct *)globalcontext)->numFitRedCoeff);
	int FitDMCoeff=2*(((MNStruct *)globalcontext)->numFitDMCoeff);
	int FitBandCoeff=2*(((MNStruct *)globalcontext)->numFitBandNoiseCoeff);
	int FitGroupNoiseCoeff = 2*((MNStruct *)globalcontext)->numFitGroupNoiseCoeff;

	int totCoeff=((MNStruct *)globalcontext)->totCoeff;




	double *powercoeff=new double[totCoeff];
	for(int o=0;o<totCoeff; o++){
		powercoeff[o]=0;
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Red Noise///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	double *freqs = new double[totCoeff];
	double *DMVec=new double[((MNStruct *)globalcontext)->pulse->nobs];

	
	double freqdet=0;
	int startpos=0;

	if(((MNStruct *)globalcontext)->incRED > 0 || ((MNStruct *)globalcontext)->incGWB == 1){


		if(((MNStruct *)globalcontext)->FitLowFreqCutoff == 1){
			double fLow = pow(10.0, Cube[pcount]);
			pcount++;

			double deltaLogF = 0.1;
			double RedMidFreq = 2.0;

			double RedLogDiff = log10(RedMidFreq) - log10(fLow);
			int LogLowFreqs = floor(RedLogDiff/deltaLogF);

			double RedLogSampledDiff = LogLowFreqs*deltaLogF;
			double sampledFLow = floor(log10(fLow)/deltaLogF)*deltaLogF;
			
			int freqStartpoint = 0;


			for(int i =0; i < LogLowFreqs; i++){
				((MNStruct *)globalcontext)->sampleFreq[freqStartpoint]=pow(10.0, sampledFLow + i*RedLogSampledDiff/LogLowFreqs);
				freqStartpoint++;

			}

			for(int i =0;i < FitRedCoeff/2-LogLowFreqs; i++){
				((MNStruct *)globalcontext)->sampleFreq[freqStartpoint]=i+RedMidFreq;
				freqStartpoint++;
			}

		}

                if(((MNStruct *)globalcontext)->FitLowFreqCutoff == 2){
                        double fLow = pow(10.0, Cube[pcount]);
                        pcount++;

                        for(int i =0;i < FitRedCoeff/2; i++){
                                ((MNStruct *)globalcontext)->sampleFreq[i]=((double)(i+1))*fLow;
                        }


                }

		for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
			
			freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[i]/maxtspan;
			freqs[startpos+i+FitRedCoeff/2]=freqs[startpos+i];

		}

	}


	if(((MNStruct *)globalcontext)->storeFMatrices == 0){


		for(int i=0;i<FitRedCoeff/2;i++){
			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
	
				TotalMatrix[k + (i+TimetoMargin+startpos)*((MNStruct *)globalcontext)->pulse->nobs]=cos(2*M_PI*freqs[i]*time);
				TotalMatrix[k + (i+FitRedCoeff/2+TimetoMargin+startpos)*((MNStruct *)globalcontext)->pulse->nobs] = sin(2*M_PI*freqs[i]*time);


			}
		}
	}

	if(((MNStruct *)globalcontext)->incRED==2){

    
		for (int i=0; i<FitRedCoeff/2; i++){
			int pnum=pcount;
			double pc=Cube[pcount];

			if(((MNStruct *)globalcontext)->RedPriorType ==1) { uniformpriorterm += log(pow(10.0,pc)); }

			powercoeff[i]=pow(10.0,2*pc);
			powercoeff[i+FitRedCoeff/2]=powercoeff[i];
			pcount++;
		}
		
		
	            
	    startpos = FitRedCoeff;

	}
	else if(((MNStruct *)globalcontext)->incRED==3 || ((MNStruct *)globalcontext)->incRED==4){

		
		for(int pl = 0; pl < ((MNStruct *)globalcontext)->numFitRedPL; pl ++){

			double Tspan = maxtspan;
                        double f1yr = 1.0/3.16e7;

			if(((MNStruct *)globalcontext)->FitLowFreqCutoff == 2){
				Tspan=Tspan/((MNStruct *)globalcontext)->sampleFreq[0];
			}



			double redamp=Cube[pcount];
			pcount++;
			double redindex=Cube[pcount];
			pcount++;

			
			double cornerfreq=0;
			if(((MNStruct *)globalcontext)->incRED==4){
				cornerfreq=pow(10.0, Cube[pcount])/Tspan;
				pcount++;
			}

	
			
			redamp=pow(10.0, redamp);
			if(((MNStruct *)globalcontext)->RedPriorType ==1) { uniformpriorterm +=log(redamp); }



			double Agw=redamp;
			for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
				
				
				double rho=0;
				if(((MNStruct *)globalcontext)->incRED==3){	
 					rho = (Agw*Agw/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-redindex))/(Tspan*24*60*60);
				}
				if(((MNStruct *)globalcontext)->incRED==4){
					
			rho = pow((1+(pow((1.0/365.25)/cornerfreq,redindex/2))),2)*(Agw*Agw/12.0/(M_PI*M_PI))/pow((1+(pow(freqs[i]/cornerfreq,redindex/2))),2)/(Tspan*24*60*60)*pow(f1yr,-3.0);
				}
				//if(rho > pow(10.0,15))rho=pow(10.0,15);
 				powercoeff[i]+= rho;
 				powercoeff[i+FitRedCoeff/2]+= rho;

				

			}
		}
		


		int coefftovary=0;
		double amptovary=0.0;
		if(((MNStruct *)globalcontext)->varyRedCoeff==1){
			coefftovary=int(pow(10.0,Cube[pcount]))-1;
			pcount++;
			amptovary=pow(10.0,Cube[pcount]);
			pcount++;

			powercoeff[coefftovary]=amptovary;
			powercoeff[coefftovary+FitRedCoeff/2]=amptovary;	
		}		
		

		

		startpos=FitRedCoeff;

    }


	if(((MNStruct *)globalcontext)->incGWB==1){
		double GWBAmp=pow(10.0,Cube[pcount]);
		pcount++;
		uniformpriorterm += log(GWBAmp);
		double Tspan = maxtspan;

		if(((MNStruct *)globalcontext)->FitLowFreqCutoff == 2){
			Tspan=Tspan/((MNStruct *)globalcontext)->sampleFreq[0];
		}

		double f1yr = 1.0/3.16e7;
		for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
			double rho = (GWBAmp*GWBAmp/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-4.333))/(Tspan*24*60*60);
			powercoeff[i]+= rho;
			powercoeff[i+FitRedCoeff/2]+= rho;
			//printf("%i %g %g \n", i, freqs[i], powercoeff[i]);
		}

		startpos=FitRedCoeff;
	}

	for (int i=0; i<FitRedCoeff/2; i++){
		freqdet=freqdet+2*log(powercoeff[i]);
	}

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract  Red Shape Events////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


	int MarginRedShapelets = ((MNStruct *)globalcontext)->MarginRedShapeCoeff;
	int totalredshapecoeff = 0;
	int badshape = 0;

	double **RedShapeletMatrix;

	double globalwidth=0;
	if(((MNStruct *)globalcontext)->incRedShapeEvent != 0){


		if(MarginRedShapelets == 1){

			badshape = 1;

			totalredshapecoeff = ((MNStruct *)globalcontext)->numRedShapeCoeff*((MNStruct *)globalcontext)->incRedShapeEvent;

			

			RedShapeletMatrix = new double*[((MNStruct *)globalcontext)->pulse->nobs];
			for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
				RedShapeletMatrix[i] = new double[totalredshapecoeff];
					for(int j = 0; j < totalredshapecoeff; j++){
						RedShapeletMatrix[i][j] = 0;
					}
				}
			}


		        for(int i =0; i < ((MNStruct *)globalcontext)->incRedShapeEvent; i++){

				int numRedShapeCoeff=((MNStruct *)globalcontext)->numRedShapeCoeff;

		                double EventPos=Cube[pcount];
				pcount++;
		                double EventWidth=Cube[pcount];
				pcount++;

				globalwidth=EventWidth;



				double *RedshapeVec=new double[numRedShapeCoeff];

				double *RedshapeNorm=new double[numRedShapeCoeff];
				for(int c=0; c < numRedShapeCoeff; c++){
					RedshapeNorm[c]=1.0/sqrt(sqrt(2.0*M_PI)*pow(2.0,c)*iter_factorial(c));
				}

				for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
					double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

					double HVal=(time-EventPos)/(sqrt(2.0)*EventWidth);
					TNothpl(numRedShapeCoeff,HVal,RedshapeVec);

					for(int c=0; c < numRedShapeCoeff; c++){
						RedShapeletMatrix[k][i*numRedShapeCoeff+c] = RedshapeNorm[c]*RedshapeVec[c]*exp(-0.5*pow((time-EventPos)/EventWidth, 2));
						if(RedShapeletMatrix[k][i*numRedShapeCoeff+c] > 0.01){ badshape = 0;}
						

					}

	
				}


				delete[] RedshapeVec;
				delete[] RedshapeNorm;

			}

		if(MarginRedShapelets == 0){

		        for(int i =0; i < ((MNStruct *)globalcontext)->incRedShapeEvent; i++){

				int numRedShapeCoeff=((MNStruct *)globalcontext)->numRedShapeCoeff;

		                double EventPos=Cube[pcount];
				pcount++;
		                double EventWidth=Cube[pcount];
				pcount++;

				globalwidth=EventWidth;


				double *Redshapecoeff=new double[numRedShapeCoeff];
				double *RedshapeVec=new double[numRedShapeCoeff];
				for(int c=0; c < numRedShapeCoeff; c++){
					Redshapecoeff[c]=Cube[pcount];
					pcount++;
				}


				double *RedshapeNorm=new double[numRedShapeCoeff];
				for(int c=0; c < numRedShapeCoeff; c++){
					RedshapeNorm[c]=1.0/sqrt(sqrt(2.0*M_PI)*pow(2.0,c)*iter_factorial(c));
				}

				for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
					double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

					double HVal=(time-EventPos)/(sqrt(2.0)*EventWidth);
					TNothpl(numRedShapeCoeff,HVal,RedshapeVec);
					double Redsignal=0;
					for(int c=0; c < numRedShapeCoeff; c++){
						Redsignal += RedshapeNorm[c]*RedshapeVec[c]*Redshapecoeff[c];
					}

					  Resvec[k] -= Redsignal*exp(-0.5*pow((time-EventPos)/EventWidth, 2));

					  //printf("Shape Sig: %i %.10Lg %g \n", k, ((MNStruct *)globalcontext)->pulse->obsn[k].bat, Redsignal*exp(-0.5*pow((time-EventPos)/EventWidth, 2)));
	
				}




			delete[] Redshapecoeff;
			delete[] RedshapeVec;
			delete[] RedshapeNorm;

			}
		}
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract Sine Wave///////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


	if(((MNStruct *)globalcontext)->incsinusoid == 1){
		double sineamp=pow(10.0,Cube[pcount]);
		pcount++;
		double sinephase=Cube[pcount];
		pcount++;
		double sinefreq=pow(10.0,Cube[pcount])/maxtspan;
		pcount++;		
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			Resvec[o]-= sineamp*sin(2*M_PI*sinefreq*(double)((MNStruct *)globalcontext)->pulse->obsn[o].bat + sinephase);
		}
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////DM Variations////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////// 


	if(((MNStruct *)globalcontext)->incDM > 0){

                for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                        DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
                }

        	for(int i=0;i<FitDMCoeff/2;i++){

			freqs[startpos+i]=((MNStruct *)globalcontext)->sampleFreq[startpos/2 - ((MNStruct *)globalcontext)->incFloatRed+i]/maxtspan;
			freqs[startpos+i+FitDMCoeff/2]=freqs[startpos+i];

			if(((MNStruct *)globalcontext)->storeFMatrices == 0){
				for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
					double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

					TotalMatrix[k + (i+TimetoMargin+startpos)*((MNStruct *)globalcontext)->pulse->nobs]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
					TotalMatrix[k + (i+FitDMCoeff/2+TimetoMargin+startpos)*((MNStruct *)globalcontext)->pulse->nobs] = sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];

				}
			}
		}
	} 


       if(((MNStruct *)globalcontext)->incDM==2){

		for (int i=0; i<FitDMCoeff/2; i++){
			int pnum=pcount;
			double pc=Cube[pcount];

			powercoeff[startpos+i]=pow(10.0,2*pc);
			powercoeff[startpos+i+FitDMCoeff/2]=powercoeff[startpos+i];
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
			pcount++;
		}
		startpos+=FitDMCoeff;
           	 
        }
        else if(((MNStruct *)globalcontext)->incDM==3){

		for(int pl = 0; pl < ((MNStruct *)globalcontext)->numFitDMPL; pl ++){
			double DMamp=Cube[pcount];
			pcount++;
			double DMindex=Cube[pcount];
			pcount++;

			
   			double Tspan = maxtspan;
			double f1yr = 1.0/3.16e7;
    			

			DMamp=pow(10.0, DMamp);
			if(((MNStruct *)globalcontext)->DMPriorType ==1) { uniformpriorterm += log(DMamp); }
			for (int i=0; i<FitDMCoeff/2; i++){
	
 				double rho = (DMamp*DMamp)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-DMindex))/(maxtspan*24*60*60);	
 				powercoeff[startpos+i]+=rho;
 				powercoeff[startpos+i+FitDMCoeff/2]+=rho;
			}
		}
		
		
		int coefftovary=0;
		double amptovary=0.0;
		if(((MNStruct *)globalcontext)->varyDMCoeff==1){
			coefftovary=int(pow(10.0,Cube[pcount]))-1;
			pcount++;
			amptovary=pow(10.0,Cube[pcount])/(maxtspan*24*60*60);
			pcount++;

			powercoeff[startpos+coefftovary]=amptovary;
			powercoeff[startpos+coefftovary+FitDMCoeff/2]=amptovary;	
		}	
			
		
		for (int i=0; i<FitDMCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}
		startpos+=FitDMCoeff;

        }




/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract  DM Shape Events////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	if(((MNStruct *)globalcontext)->incDMShapeEvent != 0){

		if(((MNStruct *)globalcontext)->incDM == 0){
			        for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                        		DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
                		}
		}
                for(int i =0; i < ((MNStruct *)globalcontext)->incDMShapeEvent; i++){

			int numDMShapeCoeff=((MNStruct *)globalcontext)->numDMShapeCoeff;

                        double EventPos=Cube[pcount];
			pcount++;
                        double EventWidth=Cube[pcount];
			pcount++;


			globalwidth=EventWidth;

			double *DMshapecoeff=new double[numDMShapeCoeff];
			double *DMshapeVec=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapecoeff[c]=Cube[pcount];
				pcount++;
			}


			double *DMshapeNorm=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapeNorm[c]=1.0/sqrt(sqrt(2.0*M_PI)*pow(2.0,c)*iter_factorial(c));
			}

			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

				double HVal=(time-EventPos)/(sqrt(2.0)*EventWidth);
				TNothpl(numDMShapeCoeff,HVal,DMshapeVec);
				double DMsignal=0;
				for(int c=0; c < numDMShapeCoeff; c++){
					DMsignal += DMshapeNorm[c]*DMshapeVec[c]*DMshapecoeff[c]*DMVec[k];
				}

				  Resvec[k] -= DMsignal*exp(-0.5*pow((time-EventPos)/EventWidth, 2));
	
			}




		delete[] DMshapecoeff;
		delete[] DMshapeVec;
		delete[] DMshapeNorm;

		}
	}





/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract  DM Scatter Shape Events///////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


	if(((MNStruct *)globalcontext)->incDMScatterShapeEvent != 0){
                for(int i =0; i < ((MNStruct *)globalcontext)->incDMScatterShapeEvent; i++){

			int numDMShapeCoeff=((MNStruct *)globalcontext)->numDMScatterShapeCoeff;

                        double EventPos=Cube[pcount];
			pcount++;
                        double EventWidth=globalwidth;//Cube[pcount];
			pcount++;
                        double EventFScale=Cube[pcount];
			pcount++;

			//EventWidth=globalwidth;

			double *DMshapecoeff=new double[numDMShapeCoeff];
			double *DMshapeVec=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapecoeff[c]=Cube[pcount];
				pcount++;
			}


			double *DMshapeNorm=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapeNorm[c]=1.0/sqrt(sqrt(2.0*M_PI)*pow(2.0,c)*iter_factorial(c));
			}

			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

				double HVal=(time-EventPos)/(sqrt(2.0)*EventWidth);
				TNothpl(numDMShapeCoeff,HVal,DMshapeVec);
				double DMsignal=0;
				for(int c=0; c < numDMShapeCoeff; c++){
					DMsignal += DMshapeNorm[c]*DMshapeVec[c]*DMshapecoeff[c]*pow(DMVec[k],EventFScale/2.0);
				}

				  Resvec[k] -= DMsignal*exp(-0.5*pow((time-EventPos)/EventWidth, 2));
	
			}




		delete[] DMshapecoeff;
		delete[] DMshapeVec;
		delete[] DMshapeNorm;

		}
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract Yearly DM//////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////// 


	if(((MNStruct *)globalcontext)->yearlyDM == 1){
		double yearlyamp=pow(10.0,Cube[pcount]);
		pcount++;
		double yearlyphase=Cube[pcount];
		pcount++;
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			Resvec[o]-= yearlyamp*sin((2*M_PI/365.25)*(double)((MNStruct *)globalcontext)->pulse->obsn[o].bat + yearlyphase)*DMVec[o];
		}
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////////Subtract Solar Wind//////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////// 

	if(((MNStruct *)globalcontext)->FitSolarWind == 1){

		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){

			Resvec[o]-= (SolarWind-((MNStruct *)globalcontext)->pulse->ne_sw)*((MNStruct *)globalcontext)->pulse->obsn[o].tdis2;
		}
	}

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Band DM/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


        if(((MNStruct *)globalcontext)->incBandNoise > 0){

                if(((MNStruct *)globalcontext)->incDM == 0){
                        for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                                DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
                        }
                }

		for(int b = 0; b < ((MNStruct *)globalcontext)->incBandNoise; b++){

			double startfreq = ((MNStruct *)globalcontext)->FitForBand[b][0];
			double stopfreq = ((MNStruct *)globalcontext)->FitForBand[b][1];
			double BandScale = ((MNStruct *)globalcontext)->FitForBand[b][2];
			int BandPriorType = ((MNStruct *)globalcontext)->FitForBand[b][3];


			double Bandamp=Cube[pcount];
			pcount++;
			double Bandindex=Cube[pcount];
			pcount++;

			double Tspan = maxtspan;
			double f1yr = 1.0/3.16e7;

			Bandamp=pow(10.0, Bandamp);
			if(BandPriorType == 1) { uniformpriorterm += log(Bandamp); }
			for (int i=0; i<FitBandCoeff/2; i++){

				freqs[startpos+i]=((double)(i+1))/maxtspan;
				freqs[startpos+i+FitBandCoeff/2]=freqs[startpos+i];
				
				double rho = (Bandamp*Bandamp/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-Bandindex))/(maxtspan*24*60*60);	
				powercoeff[startpos+i]+=rho;
				powercoeff[startpos+i+FitBandCoeff/2]+=rho;
			}
			
			
			for (int i=0; i<FitBandCoeff/2; i++){
				freqdet=freqdet+2*log(powercoeff[startpos+i]);
			}
			if(((MNStruct *)globalcontext)->storeFMatrices == 0){
				for(int i=0;i<FitBandCoeff/2;i++){
					for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
						if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
							double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
							TotalMatrix[k + (i+TimetoMargin+startpos)*((MNStruct *)globalcontext)->pulse->nobs]=cos(2*M_PI*freqs[startpos+i]*time);
							TotalMatrix[k + (i+TimetoMargin+startpos+FitBandCoeff/2)*((MNStruct *)globalcontext)->pulse->nobs]=sin(2*M_PI*freqs[startpos+i]*time);
						}
						else{	
							TotalMatrix[k + (i+TimetoMargin+startpos)*((MNStruct *)globalcontext)->pulse->nobs]=0;
							TotalMatrix[k + (i+TimetoMargin+startpos+FitBandCoeff/2)*((MNStruct *)globalcontext)->pulse->nobs]=0;

						}


					}
				}
			}

			startpos += FitBandCoeff;
		}

    	}





/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Add Group Noise/////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

        if(((MNStruct *)globalcontext)->incGroupNoise > 0){

		for(int g = 0; g < ((MNStruct *)globalcontext)->incGroupNoise; g++){

			int GrouptoFit=0;
			double GroupStartTime = 0;
			double GroupStopTime = 0;
			double GroupTSpan = 0;
			if(((MNStruct *)globalcontext)->FitForGroup[g][0] == -1){
				GrouptoFit = floor(Cube[pcount]);
				pcount++;
				//printf("Fit for group %i \n", GrouptoFit);
			}
			else{
				GrouptoFit = ((MNStruct *)globalcontext)->FitForGroup[g][0];
				
			}

                        if(((MNStruct *)globalcontext)->FitForGroup[g][1] == 1){

				double GLength = ((MNStruct *)globalcontext)->GroupStartTimes[GrouptoFit][1] - ((MNStruct *)globalcontext)->GroupStartTimes[GrouptoFit][0];
                                GroupStartTime = ((MNStruct *)globalcontext)->GroupStartTimes[GrouptoFit][0] + Cube[pcount]*GLength;
				
				double LengthLeft =  ((MNStruct *)globalcontext)->GroupStartTimes[GrouptoFit][1] - GroupStartTime;

                                pcount++;
				GroupStopTime = GroupStartTime + Cube[pcount]*LengthLeft;
                                pcount++;
				GroupTSpan = GroupStopTime-GroupStartTime;

				//printf("Start Stop %i %g %g \n", GrouptoFit, GroupStartTime, GroupStopTime);

                        }
                        else{
                                GroupStartTime = 0;
				GroupStopTime = 10000000.0;
				GroupTSpan = maxtspan;
                        }

		

			double GroupAmp=Cube[pcount];
			pcount++;
			double GroupIndex=Cube[pcount];
			pcount++;


		
			double Tspan = maxtspan;
			double f1yr = 1.0/3.16e7;
		

			GroupAmp=pow(10.0, GroupAmp);
			if(((MNStruct *)globalcontext)->DMPriorType ==1) { uniformpriorterm += log(GroupAmp); }

			for (int i=0; i<FitGroupNoiseCoeff/2; i++){

				freqs[startpos+i]=((double)(i+1.0))/GroupTSpan;//maxtspan;
				freqs[startpos+i+FitGroupNoiseCoeff/2]=freqs[startpos+i];
			
				double rho = (GroupAmp*GroupAmp/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-GroupIndex))/(maxtspan*24*60*60);	
				powercoeff[startpos+i]+=rho;
				powercoeff[startpos+i+FitGroupNoiseCoeff/2]+=rho;
			}
		
		
			
			for (int i=0; i<FitGroupNoiseCoeff/2; i++){
				freqdet=freqdet+2*log(powercoeff[startpos+i]);
			}


			for(int i=0;i<FitGroupNoiseCoeff/2;i++){
				for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
					if(((MNStruct *)globalcontext)->GroupNoiseFlags[k] == GrouptoFit && (double)((MNStruct *)globalcontext)->pulse->obsn[k].bat > GroupStartTime && (double)((MNStruct *)globalcontext)->pulse->obsn[k].bat < GroupStopTime){
			//		if(((MNStruct *)globalcontext)->GroupNoiseFlags[k] == GrouptoFit){
				       		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
						TotalMatrix[k + (i+TimetoMargin+startpos)*((MNStruct *)globalcontext)->pulse->nobs]=cos(2*M_PI*freqs[startpos+i]*time);
						TotalMatrix[k + (i+TimetoMargin+startpos+FitGroupNoiseCoeff/2)*((MNStruct *)globalcontext)->pulse->nobs]=sin(2*M_PI*freqs[startpos+i]*time);
					}
					else{
						TotalMatrix[k + (i+TimetoMargin+startpos)*((MNStruct *)globalcontext)->pulse->nobs]=0;
                                                TotalMatrix[k + (i+TimetoMargin+startpos+FitGroupNoiseCoeff/2)*((MNStruct *)globalcontext)->pulse->nobs]=0;	
					}
				}
			}



			startpos=startpos+FitGroupNoiseCoeff;
		}

    }






/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Add ECORR Coeffs////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	if(((MNStruct *)globalcontext)->incNGJitter >0){
		for(int i =0; i < ((MNStruct *)globalcontext)->numNGJitterEpochs; i++){
			powercoeff[startpos+i] = ECORRPrior[i];
			freqdet = freqdet + log(ECORRPrior[i]);
		}
	}






/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Get Time domain likelihood//////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


#ifdef HAVE_MLAPACK
	static unsigned int oldcw;
	if(((MNStruct *)globalcontext)->uselongdouble > 0 ){
		fpu_fix_start(&oldcw);
	//	printf("oldcw %i \n", oldcw);
	}
#endif	


	double tdet=0;
	double timelike=0;

	for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		//printf("Res: %i  %g %g \n", o, Resvec[o], sqrt(Noise[o]));
		timelike+=Resvec[o]*Resvec[o]*Noise[o];
		tdet -= log(Noise[o]);
	}

#ifdef HAVE_MLAPACK
	dd_real ddtimelike=0.0;
	if(((MNStruct *)globalcontext)->uselongdouble ==1){
		for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			dd_real res = (dd_real)Resvec[o];
			dd_real Nval = (dd_real)Noise[o];
			dd_real chicomp = res*res*Nval;
			ddtimelike += chicomp;
		}
	}
	//printf("timelike %g %g \n", timelike, cast2double(ddtimelike));
	
        qd_real qdtimelike=0.0;
        if(((MNStruct *)globalcontext)->uselongdouble ==2){
                for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){

                        qd_real res = (qd_real)Resvec[o];
                        qd_real Nval = (qd_real)Noise[o];
                        qd_real chicomp = res*res*Nval;

                        qdtimelike+=chicomp;
                }
		//printf("qdtimelike %15.10e\n", qdtimelike.x[0]);
        }
#endif	


//////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////Form Total Matrices////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

	for(int i =0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		for(int j =0;j<totalredshapecoeff; j++){
			TotalMatrix[i + (j+TimetoMargin+totCoeff)*((MNStruct *)globalcontext)->pulse->nobs]=RedShapeletMatrix[i][j];
		}
	}


//////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////Do Algebra/////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

	int savememory = 0;
	
	double *NTd = new double[totalsize];
	double *TNT=new double[totalsize*totalsize];
	double *NT;	

	if(savememory == 0){
		NT=new double[((MNStruct *)globalcontext)->pulse->nobs*totalsize];
		std::memcpy(NT, TotalMatrix, ((MNStruct *)globalcontext)->pulse->nobs*totalsize*sizeof(double));
	
		for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
			for(int j=0;j<totalsize;j++){
				NT[i + j*((MNStruct *)globalcontext)->pulse->nobs] *= Noise[i];
			}
		}

		vector_dgemm(TotalMatrix, NT , TNT, ((MNStruct *)globalcontext)->pulse->nobs, totalsize, ((MNStruct *)globalcontext)->pulse->nobs, totalsize, 'T', 'N');

		vector_dgemv(NT,Resvec,NTd,((MNStruct *)globalcontext)->pulse->nobs,totalsize,'T');
	}
	if(savememory == 1){

		for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
			for(int j=0;j<totalsize;j++){
				TotalMatrix[i + j*((MNStruct *)globalcontext)->pulse->nobs] *= sqrt(Noise[i]);
			}
			Resvec[i] *= Noise[i];
		}

		vector_dgemm(TotalMatrix, TotalMatrix , TNT, ((MNStruct *)globalcontext)->pulse->nobs, totalsize, ((MNStruct *)globalcontext)->pulse->nobs, totalsize, 'T', 'N');

		for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
			for(int j=0;j<totalsize;j++){
				TotalMatrix[i + j*((MNStruct *)globalcontext)->pulse->nobs] /= sqrt(Noise[i]);
			}
		}


		vector_dgemv(TotalMatrix,Resvec,NTd,((MNStruct *)globalcontext)->pulse->nobs,totalsize,'T');

	}

#ifdef HAVE_MLAPACK
        dd_real ddfreqlike = 0.0;
        dd_real ddsigmadet = 0.0;

        dd_real ddfreqlikeChol = 0.0;
        dd_real ddsigmadetChol = 0.0;

	int ddinfo = 0;
	if(((MNStruct *)globalcontext)->uselongdouble ==1){
		mpackint n = totalsize;
		mpackint m = ((MNStruct *)globalcontext)->pulse->nobs;


		dd_real *A = new dd_real[m * n];
		dd_real *B = new dd_real[m * n];
		dd_real *C = new dd_real[n * n];
		dd_real *CholC = new dd_real[n * n];

		dd_real *ddRes = new dd_real[m];
		dd_real *ddNTd = new dd_real[n];
		dd_real *ddNTdCopy = new dd_real[n];
		dd_real *ddPC = new dd_real[totCoeff];
		dd_real alpha, beta;


		for (int i=0; i<m; i++) for (int j=0; j<n; j++) A[i+j*m] = (dd_real) TotalMatrix[i+j*m];
		for (int i=0; i<m; i++) for (int j=0; j<n; j++) B[i+j*m] = (dd_real) NT[i+j*m];
		for(int i =0; i < m; i++) ddRes[i] = (dd_real)Resvec[i];
		for(int i =0; i < totCoeff; i++) ddPC[i] = (dd_real) (1.0/powercoeff[i]);

		alpha = 1.0;
		beta =  0.0;
		Rgemv("t", m,n,alpha, B, m, ddRes, 1, beta, ddNTd, 1);
		for(int i =0; i < n; i++) {
			ddNTdCopy[i] = ddNTd[i];
			//printf("ddNT %i %g \n", i, cast2double(ddNTd[i]));
		}


		Rgemm("t", "n", n, n, m, alpha, A, m, B, m, beta, C, n);



		for(int j=0;j<totCoeff;j++){
			int l = TimetoMargin+j;
			C[l+l*n] += ddPC[j];
		}

		for(int j=0;j<totalredshapecoeff;j++){
			int l = TimetoMargin+totCoeff+j;
			dd_real detfac = (dd_real)pow(10.0, -12);
			C[l+l*n] += detfac*C[l+l*n];
			
		}

		for(int i =0; i < n*n; i++)CholC[i] = C[i];

		mpackint lwork, info;

    		mpackint *ipiv = new mpackint[n];



		Rgetrf(n, n, C, n, ipiv, &info);
		if(info != 0)ddinfo = (int)info;
		for(int i =0; i < n; i++) ddsigmadet += log(abs(C[i+i*n]));

		Rgetrs("n", n, 1, C, n, ipiv, ddNTdCopy, n, &info);

		if(info != 0)ddinfo = (int)info;

		for(int i =0; i < totalsize; i++){
			ddfreqlike += ddNTd[i]*ddNTdCopy[i];
		}


		int printResiduals=0;

		if(printResiduals==1){
			dd_real *ddSignal = new dd_real[m];
			//for(int i = 0; i < TimetoMargin; i++){ddNTdCopy[i] = 0;}
			//for(int i = TimetoMargin+FitRedCoeff; i < totalsize; i++){ddNTdCopy[i] = 0;}
			Rgemv("n", m,n,alpha, A, m, ddNTdCopy, 1, beta, ddSignal, 1);

			for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
				printf("%.10Lg %g %g \n", ((MNStruct *)globalcontext)->pulse->obsn[i].bat, (double) ((MNStruct *)globalcontext)->pulse->obsn[i].residual, cast2double(ddSignal[i]));
			}
			
			delete[] ddSignal;
		}

			
		delete[] ddPC;
		delete[] ddNTd;
		delete[] ddNTdCopy;
		delete[]C;
		delete[]A;
		delete[] ipiv;



                dd_real *ddNTdChol = new dd_real[n];
                dd_real *ddNTdCholCopy = new dd_real[n];

		Rgemv("t", m,n,alpha, B, m, ddRes, 1, beta, ddNTdChol, 1);
                for(int i =0; i < n; i++) ddNTdCholCopy[i] = ddNTdChol[i];


                mpackint Cholinfo;


                Rpotrf("u", n, CholC, n, &Cholinfo);
                if(Cholinfo != 0)ddinfo = (int)Cholinfo;
                for(int i =0; i < n; i++) ddsigmadetChol += log(abs(CholC[i+i*n]));

                Rpotrs("u", n, 1, CholC, n,  ddNTdCholCopy, n, &Cholinfo);

                if(Cholinfo != 0)ddinfo = (int)Cholinfo;

                for(int i =0; i < totalsize; i++){
                        ddfreqlikeChol += ddNTdChol[i]*ddNTdCholCopy[i];
                }


                delete[] ddNTdChol;
                delete[] ddNTdCholCopy;
                delete[]CholC;
		delete[] ddRes;
		delete[] B;
	}


        qd_real qdfreqlike = 0.0;
        qd_real qdsigmadet = 0.0;

        qd_real qdfreqlikeChol = 0.0;
        qd_real qdsigmadetChol = 0.0;

	int qdinfo = 0;
	if(((MNStruct *)globalcontext)->uselongdouble ==2){
		mpackint n = totalsize;
		mpackint m = ((MNStruct *)globalcontext)->pulse->nobs;


		qd_real *A = new qd_real[m * n];
		qd_real *B = new qd_real[m * n];
		qd_real *C = new qd_real[n * n];
		qd_real *CholC = new qd_real[n * n];


		qd_real *qdRes = new qd_real[m];
		qd_real *qdNTd = new qd_real[n];
		qd_real *qdNTdCopy = new qd_real[n];
		qd_real *qdPC = new qd_real[totCoeff];
		qd_real alpha, beta;


		for (int i=0; i<m; i++) for (int j=0; j<n; j++) A[i+j*m] = (qd_real) TotalMatrix[i+j*m];
		for (int i=0; i<m; i++) for (int j=0; j<n; j++) B[i+j*m] = (qd_real) NT[i+j*m];
		for(int i =0; i < m; i++) qdRes[i] = (qd_real) Resvec[i];
		for(int i =0; i < totCoeff; i++) qdPC[i] = (qd_real) (1.0/powercoeff[i]);
		alpha = 1.0;
		beta =  0.0;

		Rgemv("t", m,n,alpha, B, m, qdRes, 1, beta, qdNTd, 1);
                for(int i =0; i < n; i++) qdNTdCopy[i] = qdNTd[i];

		Rgemm("t", "n", n, n, m, alpha, A, m, B, m, beta, C, n);



		for(int j=0;j<totCoeff;j++){
				int l = TimetoMargin+j;
				//printf("FNF %i %25.15e %g \n", j, C[l+l*n].x[0], cast2double(qdPC[j]));
				C[l+l*n] += qdPC[j];
		}

		for(int j=0;j<totalredshapecoeff;j++){
			int l = TimetoMargin+totCoeff+j;
			qd_real detfac = (qd_real)pow(10.0, -12);
			C[l+l*n] += detfac*C[l+l*n];
			
		}

		for(int i =0; i < n*n; i++)CholC[i] = C[i];

		mpackint lwork, info;

    		mpackint *ipiv = new mpackint[n];



		Rgetrf(n, n, C, n, ipiv, &info);
		if(info != 0)qdinfo = (int)info;
		for(int i =0; i < n; i++) qdsigmadet += log(abs(C[i+i*n]));

		Rgetrs("n", n, 1, C, n, ipiv, qdNTdCopy, n, &info);

		if(info != 0)qdinfo = (int)info;

		for(int i =0; i < totalsize; i++){
			qdfreqlike += qdNTd[i]*qdNTdCopy[i];
		}

			
		delete[] qdPC;
		delete[] qdNTd;
		delete[]C;
		delete[]A;
		delete[] ipiv;


                qd_real *qdNTdChol = new qd_real[n];
                qd_real *qdNTdCholCopy = new qd_real[n];



                mpackint Cholinfo;

                Rgemv("t", m,n,alpha, B, m, qdRes, 1, beta, qdNTdChol, 1);
                for(int i =0; i < n; i++) qdNTdCholCopy[i] = qdNTdChol[i];


                Rpotrf("u", n, CholC, n, &Cholinfo);
                if(Cholinfo != 0)qdinfo = (int)Cholinfo;
                for(int i =0; i < n; i++) qdsigmadetChol += log(abs(CholC[i+i*n]));

                Rpotrs("u", n, 1, CholC, n,  qdNTdCholCopy, n, &Cholinfo);

                if(Cholinfo != 0)qdinfo = (int)Cholinfo;

                for(int i =0; i < totalsize; i++){
                        qdfreqlikeChol += qdNTdChol[i]*qdNTdCholCopy[i];
		//	printf("diff %i %g %g %g \n", i, cast2double(qdNTdCholCopy[i] - qdNTdCopy[i]), cast2double(qdNTdCholCopy[i]), cast2double(qdNTdCopy[i]));
                }

		 delete[] qdNTdCopy;
		delete[] B;
		delete[] qdRes;
                delete[] qdNTdChol;
                delete[] qdNTdCholCopy;
                delete[]CholC;

	}
#endif

	if(savememory == 0){
		delete[] NT;
	}
	for(int j=0;j<totCoeff;j++){
			TNT[TimetoMargin+j + (TimetoMargin+j)*totalsize] += 1.0/powercoeff[j];
			
	}
	for(int j=0;j<totalredshapecoeff;j++){
			freqdet=freqdet-log(pow(10.0, -12)*TNT[TimetoMargin+totCoeff+j + (TimetoMargin+totCoeff+j)*totalsize]);
			TNT[TimetoMargin+totCoeff+j + (TimetoMargin+totCoeff+j)*totalsize] += pow(10.0, -12)*TNT[TimetoMargin+totCoeff+j + (TimetoMargin+totCoeff+j)*totalsize];
			
	}

	double freqlike=0;
	double *WorkCoeff = new double[totalsize];
	double *WorkCoeff2 = new double[totalsize];
	double *TNT2=new double[totalsize*totalsize];
	
	for(int i =0; i < totalsize; i++){
		for(int j=0 ; j < totalsize; j++){
			TNT2[i + j*totalsize] = TNT[i + j*totalsize];			
		}
	}
	for(int o1=0;o1<totalsize; o1++){
		
		WorkCoeff[o1]=NTd[o1];
		WorkCoeff2[o1]=NTd[o1];
	}

	int globalinfo=0;
	int info=0;
	double jointdet = 0;	
	vector_dpotrfInfo(TNT, totalsize, jointdet, info);
	if(info != 0)globalinfo=1;

	info=0;
	vector_dpotrsInfo(TNT, WorkCoeff, totalsize, info);

	/*
	if(((MNStruct *)globalcontext)->doGrades == 1){

		for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
			((MNStruct *)globalcontext)->PreviousNoise[i] = Noise[i];
		}

		for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
			for(int j=0;j<totalsize;j++){
				((MNStruct *)globalcontext)->PreviousNT[i][j] = NT[i][j];
			}
		}

		for(int i =0; i < totalsize; i++){
			for(int j=0 ; j < totalsize; j++){
				((MNStruct *)globalcontext)->PreviousTNT[i][j] = TNT[i][j];
			}
		}
	}
*/
        if(info != 0)globalinfo=1;
	info=0;
	double det2=0;
	vector_TNqrsolve(TNT2, NTd, WorkCoeff2, totalsize, det2, info);
 
	double freqlike2 = 0;    
	for(int j=0;j<totalsize;j++){
		freqlike += NTd[j]*WorkCoeff[j];
		freqlike2+=NTd[j]*WorkCoeff2[j];
	}


	double lnewChol =-0.5*(tdet+jointdet+freqdet+timelike-freqlike) + uniformpriorterm;
	double lnew=-0.5*(tdet+det2+freqdet+timelike-freqlike2) + uniformpriorterm;
	//printf("Double %g %i\n", lnew, globalinfo);
	if(fabs(lnew-lnewChol)>0.05){
		globalinfo = 1;
	//	lnew=-pow(10.0,20);
	}
	if(isnan(lnew) || isinf(lnew) || globalinfo != 0){
		globalinfo = 1;
		lnew=-pow(10.0,20);
		
	}

#ifdef HAVE_MLAPACK
	//printf("lnew double : %g", lnew);
	if(((MNStruct *)globalcontext)->uselongdouble ==1){

		dd_real ddAllLike = -0.5*(tdet+ddsigmadet+freqdet+ddtimelike-ddfreqlike) + uniformpriorterm;
		double ddlike = cast2double(ddAllLike);
		dd_real ddAllLikeChol = -0.5*(tdet+2*ddsigmadetChol+freqdet+ddtimelike-ddfreqlikeChol) + uniformpriorterm;
		double ddlikeChol = cast2double(ddAllLikeChol);
		lnew = ddlike;
		
		if(fabs(ddlikeChol-ddlike)>0.5){
			lnew=-pow(10.0,20);
		}


		if(isnan(lnew) || isinf(lnew) || ddinfo != 0){
			lnew=-pow(10.0,20);
		}

	//	printf("double double %g %g %g\n", lnew,ddlike,  fabs(ddlikeChol-ddlike));
	}

	

        if(((MNStruct *)globalcontext)->uselongdouble ==2){

		qd_real qdAllLike = -0.5*(tdet+qdsigmadet+freqdet+qdtimelike-qdfreqlike) + uniformpriorterm;
		double qdlike = cast2double(qdAllLike);
		qd_real qdAllLikeChol = -0.5*(tdet+2*qdsigmadetChol+freqdet+qdtimelike-qdfreqlikeChol) + uniformpriorterm;
		double qdlikeChol = cast2double(qdAllLikeChol);

                lnew = qdlike;

                if(fabs(qdlikeChol-qdlike)>0.05){
                        lnew=-pow(10.0,20);
                }


                if(isnan(lnew) || isinf(lnew) || ddinfo != 0){
                        lnew=-pow(10.0,20);
                }
        }


        if(((MNStruct *)globalcontext)->uselongdouble > 0 ){
                fpu_fix_end(&oldcw);
        }
#endif
	if(badshape == 1){lnew=-pow(10.0,20);}


	delete[] DMVec;
	delete[] WorkCoeff;
	delete[] WorkCoeff2;
	for(int i=0; i < ((MNStruct *)globalcontext)->EPolTerms; i++) delete[] EFAC[i];
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;
	delete[] powercoeff;
	delete[] NTd;
	delete[] freqs;
	delete[] Noise;
	delete[] Resvec;
	delete[]TNT;
	delete[] TNT2;

	if(((MNStruct *)globalcontext)->incNGJitter >0){
		delete[] ECORRPrior;
	}

	if(totalredshapecoeff > 0){
		for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
			delete[] RedShapeletMatrix[j];
		}
		delete[] RedShapeletMatrix;
	}


        ((MNStruct *)globalcontext)->PreviousInfo = globalinfo;
        ((MNStruct *)globalcontext)->PreviousJointDet = jointdet;
        ((MNStruct *)globalcontext)->PreviousFreqDet = freqdet;
        ((MNStruct *)globalcontext)->PreviousUniformPrior = uniformpriorterm;

        // printf("tdet %g, jointdet %g, freqdet %g, lnew %g, timelike %g, freqlike %g\n", tdet, jointdet, freqdet, lnew, timelike, freqlike);


	//printf("CPUChisq: %g %g %g %g %g %g \n",lnew,jointdet,tdet,freqdet,timelike,freqlike);


	return lnew;

	


}

/*
double  LRedNumericalLogLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){

	double **EFAC;
	double *EQUAD;

	
	int numfit=((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps;
	long double LDparams[numfit];




	int fitcount=0;
	int pcount=0;
	
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){
		if(((MNStruct *)globalcontext)->Dpriors[p][1] != ((MNStruct *)globalcontext)->Dpriors[p][0]){

			double val = 0;
			if((((MNStruct *)globalcontext)->LDpriors[p][3]) == 0){
				val = Cube[fitcount];
			}
			if((((MNStruct *)globalcontext)->LDpriors[p][3]) == 1){
				val = pow(10.0,Cube[fitcount]);
			}
			LDparams[p]=val*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
			fitcount++;
		}
		else if(((MNStruct *)globalcontext)->Dpriors[p][1] == ((MNStruct *)globalcontext)->Dpriors[p][0]){
			LDparams[p]=((MNStruct *)globalcontext)->Dpriors[p][0]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		}


	}

	pcount=0;
	double phase=(double)LDparams[0];
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];	
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	
	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);      
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,1);      
	

	double *Resvec=new double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		Resvec[o]=(double)((MNStruct *)globalcontext)->pulse->obsn[o].residual+phase;

	}
	
	
	
	pcount=fitcount;


/////////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////////Subtract Steps////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	if(((MNStruct *)globalcontext)->incStep > 0){
		for(int i = 0; i < ((MNStruct *)globalcontext)->incStep; i++){
			double StepAmp = Cube[pcount];
			pcount++;
			double StepTime = Cube[pcount];
			pcount++;
			for(int o1=0;o1<((MNStruct *)globalcontext)->pulse->nobs; o1++){
				if(((MNStruct *)globalcontext)->pulse->obsn[o1].bat > StepTime){
					Resvec[o1] += StepAmp;
				}
			}
		}
	}	


/////////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////////Subtract GLitches/////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////// 


 
	for(int i = 0 ; i < ((MNStruct *)globalcontext)->incGlitch; i++){
		double GlitchMJD = Cube[pcount];
		pcount++;

		double *GlitchAmps = new double[2];
		if(((MNStruct *)globalcontext)->incGlitchTerms==1 || ((MNStruct *)globalcontext)->incGlitchTerms==3){
			GlitchAmps[0]  = Cube[pcount];
			pcount++;
		}
		else if(((MNStruct *)globalcontext)->incGlitchTerms==2){
			GlitchAmps[0]  = Cube[pcount];
                        pcount++;
			GlitchAmps[1]  = Cube[pcount];
                        pcount++;
		}


		for(int o1=0;o1<((MNStruct *)globalcontext)->pulse->nobs; o1++){
                        if(((MNStruct *)globalcontext)->pulse->obsn[o1].bat > GlitchMJD){

				if(((MNStruct *)globalcontext)->incGlitchTerms==1){

					long double arg=0;
					arg=((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0])*86400.0;
					double darg = (double)arg;
                                        Resvec[o1] += GlitchAmps[0]*darg;

				}
				else if(((MNStruct *)globalcontext)->incGlitchTerms==2){
					for(int j = 0; j < ((MNStruct *)globalcontext)->incGlitchTerms; j++){

						long double arg=0;
						if(j==0){
							arg=((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0])*86400.0;
						}
						if(j==1){
							arg=0.5*pow((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)*86400.0,2)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0];
						}
						double darg = (double)arg;
						Resvec[o1] += GlitchAmps[j]*darg;
					}
				}
				else if(((MNStruct *)globalcontext)->incGlitchTerms==3){
					long double arg=0;
					arg=0.5*pow((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)*86400.0,2)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0];
					double darg = (double)arg;
                                        Resvec[o1] += GlitchAmps[0]*darg;
				}


			}
		}
	}
	
/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Get White Noise vector///////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	double uniformpriorterm=0;

	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double*[((MNStruct *)globalcontext)->EPolTerms];
		for(int n=1; n <=((MNStruct *)globalcontext)->EPolTerms; n++){
			EFAC[n-1]=new double[((MNStruct *)globalcontext)->systemcount];
			if(n==1){
				for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
					EFAC[n-1][o]=1;
				}
			}
			else{
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                   EFAC[n-1][o]=0;
                }
			}
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double*[((MNStruct *)globalcontext)->EPolTerms];
		for(int n=1; n <=((MNStruct *)globalcontext)->EPolTerms; n++){
			
			EFAC[n-1]=new double[((MNStruct *)globalcontext)->systemcount];
			if(n==1){
				for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
					EFAC[n-1][o]=pow(10.0,Cube[pcount]);
					if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[n-1][o]);}
				}
				pcount++;
			}
			else{
                                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){

                                        EFAC[n-1][o]=pow(10.0,Cube[pcount]);
                                }
                                pcount++;
                        }
		}
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double*[((MNStruct *)globalcontext)->EPolTerms];
		for(int n=1; n <=((MNStruct *)globalcontext)->EPolTerms; n++){
			EFAC[n-1]=new double[((MNStruct *)globalcontext)->systemcount];
			if(n==1){
				for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
					EFAC[n-1][p]=pow(10.0,Cube[pcount]);
					if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[n-1][p]);}
					pcount++;
				}
			}
			else{
                                for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
                                        EFAC[n-1][p]=pow(10.0,Cube[pcount]);
                                        pcount++;
                                }
                        }
		}
	}	

		
	//printf("Equad %i \n", ((MNStruct *)globalcontext)->numFitEQUAD);
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,2*Cube[pcount]);
			if(((MNStruct *)globalcontext)->EQUADPriorType ==1) { uniformpriorterm +=log(pow(10.0,Cube[pcount]));}
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){

			if(((MNStruct *)globalcontext)->includeEQsys[o] == 1){
				//printf("Cube: %i %i %g \n", o, pcount, Cube[pcount]);
				EQUAD[o]=pow(10.0,2*Cube[pcount]);
				if(((MNStruct *)globalcontext)->EQUADPriorType ==1) { uniformpriorterm +=log(pow(10.0,Cube[pcount])); }
				pcount++;
			}
			else{
				EQUAD[o]=0;
			}
			//printf("Equad? %i %g \n", o, EQUAD[o]);
		}
    	}
    

    double *SQUAD;
	if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,2*Cube[pcount]);
			if(((MNStruct *)globalcontext)->EQUADPriorType ==1) { uniformpriorterm +=log(pow(10.0,Cube[pcount]));}
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
        SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
        for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
            SQUAD[o]=pow(10.0,2*Cube[pcount]);
	    if(((MNStruct *)globalcontext)->EQUADPriorType ==1) { uniformpriorterm +=log(pow(10.0,Cube[pcount])); }

			pcount++;
        }
    }
 
	double *ECORRPrior;
	if(((MNStruct *)globalcontext)->incNGJitter >0){
		double *ECorrCoeffs=new double[((MNStruct *)globalcontext)->incNGJitter];	
		for(int i =0; i < ((MNStruct *)globalcontext)->incNGJitter; i++){
			ECorrCoeffs[i] = pow(10.0, 2*Cube[pcount]);
			pcount++;
		}
    		ECORRPrior = new double[((MNStruct *)globalcontext)->numNGJitterEpochs];
		for(int i =0; i < ((MNStruct *)globalcontext)->numNGJitterEpochs; i++){
			ECORRPrior[i] = ECorrCoeffs[((MNStruct *)globalcontext)->NGJitterSysFlags[i]];
		}

		delete[] ECorrCoeffs;
	} 



	double *Noise;	
	double *BATvec;
	Noise=new double[((MNStruct *)globalcontext)->pulse->nobs];
	BATvec=new double[((MNStruct *)globalcontext)->pulse->nobs];
	
	
	for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		BATvec[o]=(double)((MNStruct *)globalcontext)->pulse->obsn[o].bat;
	}
		
		
	if(((MNStruct *)globalcontext)->whitemodel == 0){
	
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			double EFACterm=0;
			double noiseval=0;
			double ShannonJitterTerm=0;
			
			
			if(((MNStruct *)globalcontext)->useOriginalErrors==0){
				noiseval=((MNStruct *)globalcontext)->pulse->obsn[o].toaErr;
			}
			else if(((MNStruct *)globalcontext)->useOriginalErrors==1){
				noiseval=((MNStruct *)globalcontext)->pulse->obsn[o].origErr;
			}


			for(int n=1; n <=((MNStruct *)globalcontext)->EPolTerms; n++){
				EFACterm=EFACterm + pow((noiseval*pow(10.0,-6))/pow(pow(10.0,-7),n-1),n)*EFAC[n-1][((MNStruct *)globalcontext)->sysFlags[o]];
			}	
			
			if(((MNStruct *)globalcontext)->incShannonJitter > 0){	
			 	ShannonJitterTerm=SQUAD[((MNStruct *)globalcontext)->sysFlags[o]]*((MNStruct *)globalcontext)->TobsInfo[o]/1000.0;
			}

			Noise[o]= 1.0/(pow(EFACterm,2) + EQUAD[((MNStruct *)globalcontext)->sysFlags[o]]+ShannonJitterTerm);

		}
		
	}
	else if(((MNStruct *)globalcontext)->whitemodel == 1){
	
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){

			Noise[o]=1.0/(EFAC[0][((MNStruct *)globalcontext)->sysFlags[o]]*EFAC[0][((MNStruct *)globalcontext)->sysFlags[o]]*(pow(((((MNStruct *)globalcontext)->pulse->obsn[o].toaErr)*pow(10.0,-6)),2) + EQUAD[((MNStruct *)globalcontext)->sysFlags[o]]));
		}
		
	}

	
	

//////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////////////Set up Coefficients///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////  



	double start,end;
	int go=0;
	for (int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++)
	  {
	    if (((MNStruct *)globalcontext)->pulse->obsn[i].deleted==0)
	      {
		if (go==0)
		  {
		    go = 1;
		    start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    end  = start;
		  }
		else
		  {
		    if (start > (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    if (end < (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      end = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		  }
	      }
	  }

	double maxtspan=1*(end-start);
	double averageTSamp=2*maxtspan/((MNStruct *)globalcontext)->pulse->nobs;

	double **DMEventInfo;

	int FitRedCoeff=2*(((MNStruct *)globalcontext)->numFitRedCoeff);
	int FitDMCoeff=2*(((MNStruct *)globalcontext)->numFitDMCoeff);
	int FitBandNoiseCoeff=2*(((MNStruct *)globalcontext)->numFitBandNoiseCoeff);
	int FitGroupNoiseCoeff = 2*((MNStruct *)globalcontext)->numFitGroupNoiseCoeff;

	if(((MNStruct *)globalcontext)->incFloatDM != 0)FitDMCoeff+=2*((MNStruct *)globalcontext)->incFloatDM;
	if(((MNStruct *)globalcontext)->incFloatRed != 0)FitRedCoeff+=2*((MNStruct *)globalcontext)->incFloatRed;
	int DMEventCoeff=0;
	if(((MNStruct *)globalcontext)->incDMEvent != 0){
		DMEventInfo=new double*[((MNStruct *)globalcontext)->incDMEvent];
		for(int i =0; i < ((MNStruct *)globalcontext)->incDMEvent; i++){
			DMEventInfo[i]=new double[7];
			DMEventInfo[i][0]=Cube[pcount]; //Start time
			pcount++;
			DMEventInfo[i][1]=Cube[pcount]; //Length
			pcount++;
			DMEventInfo[i][2]=pow(10.0, Cube[pcount]); //Amplitude
			pcount++;
			DMEventInfo[i][3]=Cube[pcount]; //SpectralIndex
			pcount++;
			DMEventInfo[i][4]=Cube[pcount]; //Offset
			pcount++;
			DMEventInfo[i][5]=Cube[pcount]; //Linear
			pcount++;
			DMEventInfo[i][6]=Cube[pcount]; //Quadratic
			pcount++;
			DMEventCoeff+=2*int(DMEventInfo[i][1]/averageTSamp);

			}
	}


	int totCoeff=0;
	if(((MNStruct *)globalcontext)->incRED != 0)totCoeff+=FitRedCoeff;
	if(((MNStruct *)globalcontext)->incDM != 0)totCoeff+=FitDMCoeff;

	if(((MNStruct *)globalcontext)->incDMScatter == 1 || ((MNStruct *)globalcontext)->incDMScatter == 2  || ((MNStruct *)globalcontext)->incDMScatter == 3)totCoeff+=FitDMScatterCoeff;

	if(((MNStruct *)globalcontext)->incDMEvent != 0)totCoeff+=DMEventCoeff; 
	if(((MNStruct *)globalcontext)->incNGJitter >0)totCoeff+=((MNStruct *)globalcontext)->numNGJitterEpochs;

	if(((MNStruct *)globalcontext)->incGroupNoise > 0)totCoeff += ((MNStruct *)globalcontext)->incGroupNoise*FitGroupNoiseCoeff;





	double *powercoeff=new double[totCoeff];
	for(int o=0;o<totCoeff; o++){
		powercoeff[o]=0;
	}
	
	double **FMatrix=new double*[((MNStruct *)globalcontext)->pulse->nobs];
	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		FMatrix[i]=new double[totCoeff];
		for(int j=0;j<totCoeff;j++){
			FMatrix[i][j]=0;
		}
	}

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Red Noise///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


    double *freqs = new double[totCoeff];

    double *DMVec=new double[((MNStruct *)globalcontext)->pulse->nobs];
    double *SignalCoeff = new double[totCoeff];

	for(int i = 0; i < totCoeff; i++){
		SignalCoeff[i] = 0;
	}


    double DMKappa = 2.410*pow(10.0,-16);
    int startpos=0;
    double freqdet=0;
    double GWBAmpPrior=0;
    int badAmp=0;
    if(((MNStruct *)globalcontext)->incRED==2){

    
		for (int i=0; i<FitRedCoeff/2; i++){
			int pnum=pcount;
			double pc=Cube[pcount];

			if(((MNStruct *)globalcontext)->RedPriorType ==1) { uniformpriorterm += log(pow(10.0,pc)); }

			freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[i]/maxtspan;
			freqs[startpos+i+FitRedCoeff/2]=freqs[startpos+i];
			powercoeff[i]=pow(10.0,2*pc);
			powercoeff[i+FitRedCoeff/2]=powercoeff[i];
			freqdet=freqdet+2*log(powercoeff[i]);
			pcount++;
		}
		
		
        for(int i=0;i<FitRedCoeff/2;i++){
                for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                        double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                        FMatrix[k][i]=cos(2*M_PI*freqs[i]*time);

                }
        }

        for(int i=0;i<FitRedCoeff/2;i++){
                for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                        double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                        FMatrix[k][i+FitRedCoeff/2]=sin(2*M_PI*freqs[i]*time);
                }
        }
	            
	    startpos=FitRedCoeff;

    }
   else if(((MNStruct *)globalcontext)->incRED==5 || ((MNStruct *)globalcontext)->incRED==6){

		freqdet=0;
		if(((MNStruct *)globalcontext)->FitLowFreqCutoff == 1){
			double fLow = pow(10.0, Cube[pcount]);
			pcount++;

			double deltaLogF = 0.1;
			double RedMidFreq = 2.0;

			double RedLogDiff = log10(RedMidFreq) - log10(fLow);
			int LogLowFreqs = floor(RedLogDiff/deltaLogF);

			double RedLogSampledDiff = LogLowFreqs*deltaLogF;
			double sampledFLow = floor(log10(fLow)/deltaLogF)*deltaLogF;
			
			int freqStartpoint = 0;


			for(int i =0; i < LogLowFreqs; i++){
				((MNStruct *)globalcontext)->sampleFreq[freqStartpoint]=pow(10.0, sampledFLow + i*RedLogSampledDiff/LogLowFreqs);
				freqStartpoint++;

			}

			for(int i =0;i < FitRedCoeff/2-LogLowFreqs; i++){
				((MNStruct *)globalcontext)->sampleFreq[freqStartpoint]=i+RedMidFreq;
				freqStartpoint++;
			}

		}

                if(((MNStruct *)globalcontext)->FitLowFreqCutoff == 2){
                        double fLow = pow(10.0, Cube[pcount]);
                        pcount++;

                        for(int i =0;i < FitRedCoeff/2; i++){
                                ((MNStruct *)globalcontext)->sampleFreq[i]=((double)(i+1))*fLow;
                        }

                }



		for(int i = 0; i < FitRedCoeff; i++){
			SignalCoeff[startpos + i] = Cube[pcount];
			pcount++;
		}
			
		
		for(int pl = 0; pl < ((MNStruct *)globalcontext)->numFitRedPL; pl ++){

			double Tspan = maxtspan;
                        double f1yr = 1.0/3.16e7;


			double redamp=Cube[pcount];
			pcount++;
			double redindex=Cube[pcount];
			pcount++;

			
			double cornerfreq=0;
			if(((MNStruct *)globalcontext)->incRED==4){
				cornerfreq=pow(10.0, Cube[pcount])/Tspan;
				pcount++;
			}

	
			
			redamp=pow(10.0, redamp);
			if(((MNStruct *)globalcontext)->RedPriorType ==1) { uniformpriorterm +=log(redamp); }



			double Agw=redamp;
			for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
				
				freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[i]/maxtspan;
				freqs[startpos+i+FitRedCoeff/2]=freqs[startpos+i];
				
				double rho=0;
				if(((MNStruct *)globalcontext)->incRED==5){	
 					rho = (Agw*Agw/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-redindex))/(maxtspan*24*60*60);
				}
				if(((MNStruct *)globalcontext)->incRED==6){
					
			rho = pow((1+(pow((1.0/365.25)/cornerfreq,redindex/2))),2)*(Agw*Agw/12.0/(M_PI*M_PI))/pow((1+(pow(freqs[i]/cornerfreq,redindex/2))),2)/(maxtspan*24*60*60)*pow(f1yr,-3.0);
				}
				//if(rho > pow(10.0,15))rho=pow(10.0,15);
 				powercoeff[i]+= rho;
 				powercoeff[i+FitRedCoeff/2]+= rho;

				

			}
		}
		


		int coefftovary=0;
		double amptovary=0.0;
		if(((MNStruct *)globalcontext)->varyRedCoeff==1){
			coefftovary=int(pow(10.0,Cube[pcount]))-1;
			pcount++;
			amptovary=pow(10.0,Cube[pcount]);
			pcount++;

			powercoeff[coefftovary]=amptovary;
			powercoeff[coefftovary+FitRedCoeff/2]=amptovary;	
		}		
		
		double GWBAmp=0;
		if(((MNStruct *)globalcontext)->incGWB==1){
			GWBAmp=pow(10.0,Cube[pcount]);
			pcount++;
			GWBAmpPrior=log(GWBAmp);
			//uniformpriorterm +=GWBAmpPrior;
			double Tspan = maxtspan;
			double f1yr = 1.0/3.16e7;
			for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
				double rho = (GWBAmp*GWBAmp/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-4.333))/(maxtspan*24*60*60);
				powercoeff[i]+= rho;
				powercoeff[i+FitRedCoeff/2]+= rho;
			}
		}

		for (int i=0; i<FitRedCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[i]);
		}
		
		for(int i=0;i<FitRedCoeff/2;i++){
			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
				FMatrix[k][i]=cos(2*M_PI*freqs[i]*time);

			}
		}

		for(int i=0;i<FitRedCoeff/2;i++){
			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
				FMatrix[k][i+FitRedCoeff/2]=sin(2*M_PI*freqs[i]*time);
			}
		}

		startpos=FitRedCoeff;

    }


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract  Red Shape Events////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	double globalwidth=0;
	if(((MNStruct *)globalcontext)->incRedShapeEvent != 0){
                for(int i =0; i < ((MNStruct *)globalcontext)->incRedShapeEvent; i++){

			int numRedShapeCoeff=((MNStruct *)globalcontext)->numRedShapeCoeff;

                        double EventPos=Cube[pcount];
			pcount++;
                        double EventWidth=Cube[pcount];
			pcount++;

			globalwidth=EventWidth;


			double *Redshapecoeff=new double[numRedShapeCoeff];
			double *RedshapeVec=new double[numRedShapeCoeff];
			for(int c=0; c < numRedShapeCoeff; c++){
				Redshapecoeff[c]=Cube[pcount];
				pcount++;
			}


			double *RedshapeNorm=new double[numRedShapeCoeff];
			for(int c=0; c < numRedShapeCoeff; c++){
				RedshapeNorm[c]=1.0/sqrt(sqrt(2.0*M_PI)*pow(2.0,c)*iter_factorial(c));
			}

			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

				double HVal=(time-EventPos)/(sqrt(2.0)*EventWidth);
				TNothpl(numRedShapeCoeff,HVal,RedshapeVec);
				double Redsignal=0;
				for(int c=0; c < numRedShapeCoeff; c++){
					Redsignal += RedshapeNorm[c]*RedshapeVec[c]*Redshapecoeff[c];
				}

				  Resvec[k] -= Redsignal*exp(-0.5*pow((time-EventPos)/EventWidth, 2));
	
			}




		delete[] Redshapecoeff;
		delete[] RedshapeVec;
		delete[] RedshapeNorm;

		}
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract Sine Wave///////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


	if(((MNStruct *)globalcontext)->incsinusoid == 1){
		double sineamp=pow(10.0,Cube[pcount]);
		pcount++;
		double sinephase=Cube[pcount];
		pcount++;
		double sinefreq=pow(10.0,Cube[pcount])/maxtspan;
		pcount++;		
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			Resvec[o]-= sineamp*sin(2*M_PI*sinefreq*(double)((MNStruct *)globalcontext)->pulse->obsn[o].bat + sinephase);
		}
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////DM Variations////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


       if(((MNStruct *)globalcontext)->incDM==2){

			for (int i=0; i<FitDMCoeff/2; i++){
				int pnum=pcount;
				double pc=Cube[pcount];
				freqs[startpos+i]=((MNStruct *)globalcontext)->sampleFreq[startpos/2 - ((MNStruct *)globalcontext)->incFloatRed+i]/maxtspan;
				freqs[startpos+i+FitDMCoeff/2]=freqs[startpos+i];
	
				powercoeff[startpos+i]=pow(10.0,pc)/(maxtspan*24*60*60);
				powercoeff[startpos+i+FitDMCoeff/2]=powercoeff[startpos+i];
				freqdet=freqdet+2*log(powercoeff[startpos+i]);
				pcount++;
			}
           	 


                for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                        DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
                }
                
            for(int i=0;i<FitDMCoeff/2;i++){
                for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                        double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                        FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                }
        }

        for(int i=0;i<FitDMCoeff/2;i++){
                for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                        double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                        FMatrix[k][startpos+i+FitDMCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                }
        }

        }
        else if(((MNStruct *)globalcontext)->incDM==5){


                for(int i = 0; i < FitDMCoeff; i++){
                        SignalCoeff[startpos + i] = Cube[pcount];
                        pcount++;
                }

		for(int pl = 0; pl < ((MNStruct *)globalcontext)->numFitDMPL; pl ++){
			double DMamp=Cube[pcount];
			pcount++;
			double DMindex=Cube[pcount];
			pcount++;

			
   			double Tspan = maxtspan;
			double f1yr = 1.0/3.16e7;
    			

			DMamp=pow(10.0, DMamp);
			if(((MNStruct *)globalcontext)->DMPriorType ==1) { uniformpriorterm += log(DMamp); }
			for (int i=0; i<FitDMCoeff/2; i++){
	
				freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[startpos/2 - ((MNStruct *)globalcontext)->incFloatRed +i]/maxtspan;
				freqs[startpos+i+FitDMCoeff/2]=freqs[startpos+i];
				
 				double rho = (DMamp*DMamp)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-DMindex))/(maxtspan*24*60*60);	
 				powercoeff[startpos+i]+=rho;
 				powercoeff[startpos+i+FitDMCoeff/2]+=rho;
			}
		}
		
		
		int coefftovary=0;
		double amptovary=0.0;
		if(((MNStruct *)globalcontext)->varyDMCoeff==1){
			coefftovary=int(pow(10.0,Cube[pcount]))-1;
			pcount++;
			amptovary=pow(10.0,Cube[pcount])/(maxtspan*24*60*60);
			pcount++;

			powercoeff[startpos+coefftovary]=amptovary;
			powercoeff[startpos+coefftovary+FitDMCoeff/2]=amptovary;	
		}	
			
		
		for (int i=0; i<FitDMCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}


        for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
        }

        for(int i=0;i<FitDMCoeff/2;i++){
                for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                        double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                        FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                }
        }

        for(int i=0;i<FitDMCoeff/2;i++){
                for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                        double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                        FMatrix[k][startpos+i+FitDMCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                }
        }

	startpos+=FitDMCoeff;

    }
    

    


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////DM Power Spectrum Events////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	
	if(((MNStruct *)globalcontext)->incDMEvent != 0){
                for(int i =0; i < ((MNStruct *)globalcontext)->incDMEvent; i++){
                        double DMamp=DMEventInfo[i][2];
                        double DMindex=DMEventInfo[i][3];
			double DMOff=DMEventInfo[i][4];
			double DMLinear=0;//DMEventInfo[i][5];
			double DMQuad=0;//DMEventInfo[i][6];

                        double Tspan = DMEventInfo[i][1];
                        double f1yr = 1.0/3.16e7;
			int DMEventnumCoeff=int(Tspan/averageTSamp);

                        for (int c=0; c<DMEventnumCoeff; c++){
                                freqs[startpos+c]=((double)(c+1))/Tspan;
                                freqs[startpos+c+DMEventnumCoeff]=freqs[startpos+c];

                                double rho = (DMamp*DMamp)*pow(f1yr,(-3)) * pow(freqs[startpos+c]*365.25,(-DMindex))/(maxtspan*24*60*60);
                                powercoeff[startpos+c]+=rho;
                                powercoeff[startpos+c+DMEventnumCoeff]+=rho;
				freqdet=freqdet+2*log(powercoeff[startpos+c]);
                        }


			double *DMshapecoeff=new double[3];
			double *DMshapeVec=new double[3];
			DMshapecoeff[0]=DMOff;
			DMshapecoeff[1]=DMLinear;			
			DMshapecoeff[2]=DMQuad;



			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;


				if(time < DMEventInfo[i][0]+Tspan && time > DMEventInfo[i][0]){

					Resvec[k] -= DMOff*DMVec[k];
					Resvec[k] -= (time-DMEventInfo[i][0])*DMLinear*DMVec[k];
					Resvec[k] -= pow((time-DMEventInfo[i][0]),2)*DMQuad*DMVec[k];

				}
			}

		 	for(int c=0;c<DMEventnumCoeff;c++){
                		for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                        		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
					if(time < DMEventInfo[i][0]+Tspan && time > DMEventInfo[i][0]){
						FMatrix[k][startpos+c]=cos(2*M_PI*freqs[startpos+c]*time)*DMVec[k];
                        			FMatrix[k][startpos+c+DMEventnumCoeff]=sin(2*M_PI*freqs[startpos+c]*time)*DMVec[k];
					}
					else{
						FMatrix[k][startpos+c]=0;
						FMatrix[k][startpos+c+DMEventnumCoeff]=0;
					}
                		}
		        }

			startpos+=2*DMEventnumCoeff;



		}
	}



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract  DM Shape Events////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	if(((MNStruct *)globalcontext)->incDMShapeEvent != 0){
                for(int i =0; i < ((MNStruct *)globalcontext)->incDMShapeEvent; i++){

			int numDMShapeCoeff=((MNStruct *)globalcontext)->numDMShapeCoeff;

                        double EventPos=Cube[pcount];
			pcount++;
                        double EventWidth=Cube[pcount];
			pcount++;


			globalwidth=EventWidth;

			double *DMshapecoeff=new double[numDMShapeCoeff];
			double *DMshapeVec=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapecoeff[c]=Cube[pcount];
				pcount++;
			}


			double *DMshapeNorm=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapeNorm[c]=1.0/sqrt(sqrt(2.0*M_PI)*pow(2.0,c)*iter_factorial(c));
			}

			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

				double HVal=(time-EventPos)/(sqrt(2.0)*EventWidth);
				TNothpl(numDMShapeCoeff,HVal,DMshapeVec);
				double DMsignal=0;
				for(int c=0; c < numDMShapeCoeff; c++){
					DMsignal += DMshapeNorm[c]*DMshapeVec[c]*DMshapecoeff[c]*DMVec[k];
				}

				  Resvec[k] -= DMsignal*exp(-0.5*pow((time-EventPos)/EventWidth, 2));
	
			}




		delete[] DMshapecoeff;
		delete[] DMshapeVec;
		delete[] DMshapeNorm;

		}
	}





/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract  DM Scatter Shape Events///////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


	if(((MNStruct *)globalcontext)->incDMScatterShapeEvent != 0){
                for(int i =0; i < ((MNStruct *)globalcontext)->incDMScatterShapeEvent; i++){

			int numDMShapeCoeff=((MNStruct *)globalcontext)->numDMScatterShapeCoeff;

                        double EventPos=Cube[pcount];
			pcount++;
                        double EventWidth=globalwidth;//Cube[pcount];
			pcount++;
                        double EventFScale=Cube[pcount];
			pcount++;

			//EventWidth=globalwidth;

			double *DMshapecoeff=new double[numDMShapeCoeff];
			double *DMshapeVec=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapecoeff[c]=Cube[pcount];
				pcount++;
			}


			double *DMshapeNorm=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapeNorm[c]=1.0/sqrt(sqrt(2.0*M_PI)*pow(2.0,c)*iter_factorial(c));
			}

			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

				double HVal=(time-EventPos)/(sqrt(2.0)*EventWidth);
				TNothpl(numDMShapeCoeff,HVal,DMshapeVec);
				double DMsignal=0;
				for(int c=0; c < numDMShapeCoeff; c++){
					DMsignal += DMshapeNorm[c]*DMshapeVec[c]*DMshapecoeff[c]*pow(DMVec[k],EventFScale/2.0);
				}

				  Resvec[k] -= DMsignal*exp(-0.5*pow((time-EventPos)/EventWidth, 2));
	
			}




		delete[] DMshapecoeff;
		delete[] DMshapeVec;
		delete[] DMshapeNorm;

		}
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract Yearly DM//////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////// 


	if(((MNStruct *)globalcontext)->yearlyDM == 1){
		double yearlyamp=pow(10.0,Cube[pcount]);
		pcount++;
		double yearlyphase=Cube[pcount];
		pcount++;
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			Resvec[o]-= yearlyamp*sin((2*M_PI/365.25)*(double)((MNStruct *)globalcontext)->pulse->obsn[o].bat + yearlyphase)*DMVec[o];
		}
	}





/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Band DM/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


        if(((MNStruct *)globalcontext)->incDMScatter == 1 || ((MNStruct *)globalcontext)->incDMScatter == 2  || ((MNStruct *)globalcontext)->incDMScatter == 3 ){

                if(((MNStruct *)globalcontext)->incDM == 0){
                        for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                                DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
                        }
                }



		double startfreq=0;
		double stopfreq=0;


		if(((MNStruct *)globalcontext)->incDMScatter == 1){		
			startfreq = 0;
			stopfreq=1000;
		}
		else  if(((MNStruct *)globalcontext)->incDMScatter == 2){
	                startfreq = 1000;
	                stopfreq=1800;
		}
	        else if(((MNStruct *)globalcontext)->incDMScatter == 3){
	                startfreq = 1800;
	                stopfreq=10000;
	        }



		double DMamp=Cube[pcount];
		pcount++;
		double DMindex=Cube[pcount];
		pcount++;

		double Tspan = maxtspan;
		double f1yr = 1.0/3.16e7;

		DMamp=pow(10.0, DMamp);
		if(((MNStruct *)globalcontext)->DMPriorType ==1) { uniformpriorterm += log(DMamp); }
		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
			
			double rho = (DMamp*DMamp)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-DMindex))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}

		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
				}
				else{	
					FMatrix[k][startpos+i]=0;	
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}


		startpos += FitDMScatterCoeff;

    	}


        if(((MNStruct *)globalcontext)->incDMScatter == 4 ){


                if(((MNStruct *)globalcontext)->incDM == 0){
                        for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                                DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
                        }
                }



		double Amp1=Cube[pcount];
		pcount++;
		double index1=Cube[pcount];
		pcount++;

		double Amp2=Cube[pcount];
		pcount++;
		double index2=Cube[pcount];
		pcount++;

		double Amp3=Cube[pcount];
		pcount++;
		double index3=Cube[pcount];
		pcount++;
		
		double Tspan = maxtspan;
		double f1yr = 1.0/3.16e7;
		

		Amp1=pow(10.0, Amp1);
		Amp2=pow(10.0, Amp2);
		Amp3=pow(10.0, Amp3);

		if(((MNStruct *)globalcontext)->DMPriorType ==1) { uniformpriorterm += log(Amp1) + log(Amp2) + log(Amp3); }


		double startfreq=0;
		double stopfreq=0;

		/////////////////////////Amp1/////////////////////////////////////////////////////////////////////////////////////////////

		startfreq = 0;
		stopfreq=1000;

		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1.0))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
			
			double rho = (Amp1*Amp1)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-index1))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
			
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}

		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time);//*DMVec[k];
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time);//*DMVec[k];
				}
				else{	
					FMatrix[k][startpos+i]=0;
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}


		startpos=startpos+FitDMScatterCoeff;

		/////////////////////////Amp2/////////////////////////////////////////////////////////////////////////////////////////////

		startfreq = 1000;
		stopfreq=2000;

		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1.0))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
			
			double rho = (Amp2*Amp2)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-index2))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
			
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}


		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time);//*DMVec[k];
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time);//*DMVec[k];
				}
				else{	
					FMatrix[k][startpos+i]=0;
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}


		startpos=startpos+FitDMScatterCoeff;

		/////////////////////////Amp3/////////////////////////////////////////////////////////////////////////////////////////////


		startfreq = 2000;
		stopfreq=10000;

		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1.0))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
			
			double rho = (Amp3*Amp3)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-index3))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
			
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}


		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time);//*DMVec[k];
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time);//*DMVec[k];
				}
				else{	
					FMatrix[k][startpos+i]=0;
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}

		startpos=startpos+FitDMScatterCoeff;
	}	


        if(((MNStruct *)globalcontext)->incDMScatter == 5 ){

	        
		if(((MNStruct *)globalcontext)->incDM == 0){
			for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		       		DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
	      		}
		}

		double Amp1=Cube[pcount];
		pcount++;
		double index1=Cube[pcount];
		pcount++;


		double Tspan = maxtspan;
		double f1yr = 1.0/3.16e7;
		

		Amp1=pow(10.0, Amp1);

		if(((MNStruct *)globalcontext)->DMPriorType ==1) { uniformpriorterm += log(Amp1); }


		double startfreq=0;
		double stopfreq=0;

		/////////////////////////50cm/////////////////////////////////////////////////////////////////////////////////////////////

		startfreq = 0;
		stopfreq=1000;

		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1.0))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
			
			double rho = (Amp1*Amp1)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-index1))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
			
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}

		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
				}
				else{	
					FMatrix[k][startpos+i]=0;
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}


		startpos=startpos+FitDMScatterCoeff;

		/////////////////////////20cm/////////////////////////////////////////////////////////////////////////////////////////////

		startfreq = 1000;
		stopfreq=1800;

		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1.0))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
			
			double rho = (Amp1*Amp1)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-index1))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
			
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}


		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
				}
				else{	
					FMatrix[k][startpos+i]=0;
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}


		startpos=startpos+FitDMScatterCoeff;

		/////////////////////////10cm/////////////////////////////////////////////////////////////////////////////////////////////


		startfreq = 1800;
		stopfreq=10000;

		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1.0))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
			
			double rho = (Amp1*Amp1)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-index1))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
			
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}


		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
				}
				else{	
					FMatrix[k][startpos+i]=0;
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}

		startpos=startpos+FitDMScatterCoeff;
	}	





/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Add Group Noise/////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

        if(((MNStruct *)globalcontext)->incGroupNoise > 0){

		for(int g = 0; g < ((MNStruct *)globalcontext)->incGroupNoise; g++){

			int GrouptoFit=0;
			double GroupStartTime = 0;
			double GroupStopTime = 0;
			if(((MNStruct *)globalcontext)->FitForGroup[g][0] == -1){
				GrouptoFit = floor(Cube[pcount]);
				pcount++;
			}
			else{
				GrouptoFit = ((MNStruct *)globalcontext)->FitForGroup[g][0];
				
			}

                        if(((MNStruct *)globalcontext)->FitForGroup[g][1] == 1){
                                GroupStartTime = Cube[pcount];
                                pcount++;
				GroupStopTime = Cube[pcount];
                                pcount++;

                        }
                        else{
                                GroupStartTime = 0;
				GroupStopTime = 10000000.0;

                        }

		

			double GroupAmp=Cube[pcount];
			pcount++;
			double GroupIndex=Cube[pcount];
			pcount++;


		
			double Tspan = maxtspan;
			double f1yr = 1.0/3.16e7;
		

			GroupAmp=pow(10.0, GroupAmp);
			if(((MNStruct *)globalcontext)->DMPriorType ==1) { uniformpriorterm += log(GroupAmp); }

			for (int i=0; i<FitGroupNoiseCoeff/2; i++){

				freqs[startpos+i]=((double)(i+1.0))/maxtspan;
				freqs[startpos+i+FitGroupNoiseCoeff/2]=freqs[startpos+i];
			
				double rho = (GroupAmp*GroupAmp/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-GroupIndex))/(maxtspan*24*60*60);	
				powercoeff[startpos+i]+=rho;
				powercoeff[startpos+i+FitGroupNoiseCoeff/2]+=rho;
			}
		
		
			
			for (int i=0; i<FitGroupNoiseCoeff/2; i++){
				freqdet=freqdet+2*log(powercoeff[startpos+i]);
			}


			for(int i=0;i<FitGroupNoiseCoeff/2;i++){
				for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
					//printf("Groups %i %i %i %i\n", i,k,GrouptoFit,((MNStruct *)globalcontext)->sysFlags[k]);
				if(((MNStruct *)globalcontext)->GroupNoiseFlags[k] == GrouptoFit && (double)((MNStruct *)globalcontext)->pulse->obsn[k].bat > GroupStartTime && (double)((MNStruct *)globalcontext)->pulse->obsn[k].bat < GroupStopTime){
				//	if(((MNStruct *)globalcontext)->GroupNoiseFlags[k] == GrouptoFit){
				       		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

				        	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time);
						FMatrix[k][startpos+i+FitGroupNoiseCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time);
					}
					else{	
						FMatrix[k][startpos+i]=0;
						FMatrix[k][startpos+i+FitGroupNoiseCoeff/2]=0;
					}
				}
			}



			startpos=startpos+FitGroupNoiseCoeff;
		}

    }


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Add ECORR Coeffs////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	if(((MNStruct *)globalcontext)->incNGJitter >0){
		for(int i =0; i < ((MNStruct *)globalcontext)->numNGJitterEpochs; i++){
			powercoeff[startpos+i] = ECORRPrior[i];
			freqdet = freqdet + log(ECORRPrior[i]);
		}

		for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
			for(int i=0; i < ((MNStruct *)globalcontext)->numNGJitterEpochs; i++){
				FMatrix[k][startpos+i] = ((MNStruct *)globalcontext)->NGJitterMatrix[k][i];
			}
		}

	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Make stochastic signal//////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	double *SignalVec = new double[((MNStruct *)globalcontext)->pulse->nobs];

	
	dgemv(FMatrix,SignalCoeff,SignalVec,((MNStruct *)globalcontext)->pulse->nobs,totCoeff,'N');

	for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){


		Resvec[o] = Resvec[o] - SignalVec[o];
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Get Time domain likelihood//////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////// 

 
	static unsigned int oldcw;
	if(((MNStruct *)globalcontext)->uselongdouble > 0 ){
		fpu_fix_start(&oldcw);
	}
	


	double tdet=0;
	double timelike=0;

	for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		timelike+=Resvec[o]*Resvec[o]*Noise[o];
		tdet -= log(Noise[o]);
	}
	dd_real ddtimelike=0.0;
	if(((MNStruct *)globalcontext)->uselongdouble ==1){
		for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			dd_real res = (dd_real)Resvec[o];
			dd_real Nval = (dd_real)Noise[o];
			dd_real chicomp = res*res*Nval;
			ddtimelike += chicomp;
		}
	}

	
        qd_real qdtimelike=0.0;
        if(((MNStruct *)globalcontext)->uselongdouble ==2){
                for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){

                        qd_real res = (qd_real)Resvec[o];
                        qd_real Nval = (qd_real)Noise[o];
                        qd_real chicomp = res*res*Nval;

                        qdtimelike+=chicomp;
                }
        }


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Get Fourier domain likelihood///////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////// 


	double freqlike = 0;
	for(int i = 0; i < totCoeff; i++){
		freqlike += SignalCoeff[i]*SignalCoeff[i]/powercoeff[i];
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Get Final likelihood////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////// 



	double lnew=-0.5*(tdet+freqdet+timelike+freqlike) + uniformpriorterm;
	//printf("like, %g %g %g %g %g %g \n", lnew, tdet, freqdet, timelike, freqlike, uniformpriorterm);

	if(isnan(lnew) || isinf(lnew)){

		lnew=-pow(10.0,20);
		
	}



        if(((MNStruct *)globalcontext)->uselongdouble > 0 ){
                fpu_fix_end(&oldcw);
        }



	delete[] DMVec;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;
	delete[] powercoeff;
	delete[] freqs;
	delete[] Noise;
	delete[] Resvec;
	delete[] SignalCoeff;
	delete[] SignalVec;
	
	for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
		delete[]FMatrix[j];
	}
	delete[]FMatrix;

	if(((MNStruct *)globalcontext)->incNGJitter >0){
		delete[] ECORRPrior;
	}

	//printf("CPUChisq: %g %g %g %g %g %g \n",lnew,jointdet,tdet,freqdet,timelike,freqlike);
	return lnew;


}

*/
/*
double  AllTOAJitterLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){



	double *EFAC;
	double *EQUAD;
        long double LDparams[ndim];
        int pcount;

	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){

		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

	}	
	
	
	pcount=0;
	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];	
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
           	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
                	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){
                        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
				//printf("Jump: %i %i %i %.25Lg \n", p, k, l, JumpVec[p]);
			}
                 }
           }

	}


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		//if(p==0){printf("before: %.25Lg %.25Lg %.25Lg \n", ((MNStruct *)globalcontext)->pulse->obsn[p].origsat, ((MNStruct *)globalcontext)->pulse->obsn[p].sat, ((MNStruct *)globalcontext)->pulse->obsn[p].bat);}
		//printf("sat: %.25Lg %.25Lg %.25Lg \n", ((MNStruct *)globalcontext)->pulse->obsn[p].origsat, ((MNStruct *)globalcontext)->pulse->obsn[p].sat, phase);
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}


        for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
                ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }

	delete[] JumpVec;

	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);       

	  
	//for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
	//	printf("res: %i %.25Lg %.25Lg \n", p, ((MNStruct *)globalcontext)->pulse->obsn[p].bat, ((MNStruct *)globalcontext)->pulse->obsn[p].residual);  
	//} 
	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=1;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			pcount++;
		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
                }
        }
	  
	  
	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	   
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;
	      }
	      
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr  - phase - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
	 }


	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;

	double shapecoeff[numcoeff];

	for(int i =0; i < numcoeff; i++){

		shapecoeff[i]= Cube[pcount];
		Cube[pcount] = shapecoeff[i];
		pcount++;
	}

	double beta = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;

	//printf("beta %g %g %g \n", Cube[pcount], beta, double(((MNStruct *)globalcontext)->ReferencePeriod));

	pcount++;
	
	double lnew = 0;
	int badshape = 0;
	
	 for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
	    int nTOA = t;
	    
	    double detN  = 0;
	    double Chisq  = 0;
	    double logMargindet = 0;
	    double Marginlike = 0;	 
	    double JitterPrior = 0;	   
 
	    double profilelike=0;
	    
	    long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
	    long double FoldingPeriodDays = FoldingPeriod/SECDAY;
	    int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
	    double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
	    double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
	    long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

	    double *Betatimes = new double[Nbins];
	    double **HermiteJitterpoly =  new double*[Nbins];
	    double **JitterBasis  =  new double*[Nbins];
	    double **Hermitepoly =  new double*[Nbins];
	    for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}
            for(int i =0;i<Nbins;i++){HermiteJitterpoly[i]=new double[numcoeff+1];for(int j =0;j<numcoeff+1;j++){HermiteJitterpoly[i][j]=0;}}
	    for(int i =0;i<Nbins;i++){JitterBasis[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){JitterBasis[i][j]=0;}}
	
	



	

	    double sigma=noiseval;
	    sigma=pow(10.0, sigma)*sqrt(double(Nbins)/Tobs)*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];

	    long double binpos = ModelBats[nTOA];
	    if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

	    long double minpos = binpos - FoldingPeriodDays/2;
	    if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
	    long double maxpos = binpos + FoldingPeriodDays/2;
	    if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];


	    
	    for(int j =0; j < Nbins; j++){
		    long double timediff = 0;
		    long double bintime = ProfileBats[t][j];
		    if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
		    }
		    else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
		    }
		    else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
		    }

		    timediff=timediff*SECDAY;


		    Betatimes[j]=timediff/beta;
		    TNothpl(numcoeff+1,Betatimes[j],HermiteJitterpoly[j]);
		    for(int k =0; k < numcoeff+1; k++){
			    double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI)*((MNStruct *)globalcontext)->Factorials[k]);
			    HermiteJitterpoly[j][k]=HermiteJitterpoly[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);
			    if(k<numcoeff){ Hermitepoly[j][k] = HermiteJitterpoly[j][k];}
			
		    }

			JitterBasis[j][0] = (1.0/sqrt(2.0))*(-1.0*HermiteJitterpoly[j][1]);
		   for(int k =1; k < numcoeff; k++){
			
			JitterBasis[j][k] = (1.0/sqrt(2.0))*(sqrt(double(k))*HermiteJitterpoly[j][k-1] - sqrt(double(k+1))*HermiteJitterpoly[j][k+1]);
		   }	
	    }


	    double *shapevec  = new double[Nbins];
	    double *Jittervec = new double[Nbins];
	
	    double OverallProfileAmp = shapecoeff[0];
	    
	    shapecoeff[0] = 1;

	    dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');
	    dgemv(JitterBasis,shapecoeff,Jittervec,Nbins,numcoeff,'N');

	
	    double *NDiffVec = new double[Nbins];

	    double maxshape=0;
	    double modelflux=0;
	    double testmodelflux=0;	
            double dataflux = 0;
	//	printf("beta is %g \n", beta);

	    for(int j =0; j < Nbins; j++){

		    detN += log(sigma*sigma);

		    double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] ;//- OverallProfileAmp*shapevec[j];//-offset;

		    if(shapevec[j] < 0)badshape = 1;
		    NDiffVec[j] = datadiff/(sigma*sigma);
	           // printf("%i %g \n", j, shapevec[j]);
		    if(shapevec[j] > maxshape){ maxshape = shapevec[j];}


		    testmodelflux += shapevec[j]*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
                //    printf("%g %g %g \n", double(ProfileBats[t][1] - ProfileBats[t][0]), double(beta), double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24);
	    }
	    for(int j =0; j < numcoeff; j=j+2){
		modelflux+=sqrt(sqrt(M_PI))*sqrt(beta)*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Factorials[j]/(((MNStruct *)globalcontext)->Factorials[j - j/2]*((MNStruct *)globalcontext)->Factorials[j/2]))*shapecoeff[j];
	   }
	   //printf("fluxes %g %g \n", modelflux, testmodelflux);
	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////
	

	    double **M = new double*[Nbins];
	    double **NM = new double*[Nbins];

	    for(int i =0; i < Nbins; i++){
		    M[i] = new double[3];
		    NM[i] = new double[3];
		    
		    M[i][0] = 1;
		    M[i][1] = shapevec[i];
		    M[i][2] = Jittervec[i];

		    NM[i][0] = M[i][0]/(sigma*sigma);
		    NM[i][1] = M[i][1]/(sigma*sigma);
		    NM[i][2] = M[i][2]/(sigma*sigma);
	    }


	    double **MNM = new double*[3];
	    for(int i =0; i < 3; i++){
		    MNM[i] = new double[3];
	    }

	    dgemm(M, NM , MNM, Nbins, 3,Nbins, 3, 'T', 'N');


	    MNM[2][2] += pow(10.0,20);

	    double *dNM = new double[3];
	    double *TempdNM = new double[3];
	    dgemv(M,NDiffVec,dNM,Nbins,3,'T');

	    for(int i =0; i < 3; i++){
		TempdNM[i] = dNM[i];
	    }


            int info=0;
	    double Margindet = 0;
            dpotrfInfo(MNM, 3, Margindet, info);
            dpotrs(MNM, TempdNM, 3);

	    
	    double maxoffset = TempdNM[0];
            double maxamp = TempdNM[1];
            double maxJitter = TempdNM[2];


	    
	    if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
	      
	      
		Chisq = 0;
		detN = 0;
	      
		double profnoise = EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];//sqrt(double(Nbins)/Tobs)*EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
		profnoise = profnoise*profnoise;
		
		double MLSigma = 0;
		int MLSigmaCount = 0;
		for(int j =0; j < Nbins; j++){
		      double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - maxamp*shapevec[j] - maxoffset - maxJitter*Jittervec[j];
		      if(shapevec[j] <= maxshape*0.01){MLSigma += res*res; MLSigmaCount += 1;}
		}

		MLSigma = sqrt(MLSigma/MLSigmaCount);

		for(int j =0; j < Nbins; j++){
			if(shapevec[j] > maxshape*0.001){
				if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - maxoffset)/MLSigma > 1){
					dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-maxoffset)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
					//printf("flux: %i %i %g %g \n", t,j,dataflux,(double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-maxoffset);
				}
			}
		}	

		//printf("max: %i %g %g %g \n", t, maxamp, dataflux/modelflux, maxamp/(dataflux/modelflux));

		MLSigma = MLSigma*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];


		
		for(int j =0; j < Nbins; j++){
		  
			double noise = MLSigma*MLSigma;
		
			detN += log(noise);

			double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];

			if(shapevec[j] < 0)badshape = 1;
			NDiffVec[j] = datadiff/(noise);
		       

			Chisq += datadiff*datadiff/(noise);
			
			 NM[j][0] = M[j][0]/(noise);
			 NM[j][1] = M[j][1]/(noise);
			 
			 M[j][2] = M[j][2]*dataflux/modelflux/beta;
			 NM[j][2] = M[j][2]/(noise);
		
		}
		
		
			
		    dgemm(M, NM , MNM, Nbins, 3,Nbins, 3, 'T', 'N');


		    JitterPrior = log(profnoise);
		    MNM[2][2] += 1.0/profnoise;

		    dgemv(M,NDiffVec,dNM,Nbins,3,'T');

		    for(int i =0; i < 3; i++){
			TempdNM[i] = dNM[i];
		    }


		    info=0;
		    Margindet = 0;
		    dpotrfInfo(MNM, 3, Margindet, info);
		    dpotrs(MNM, TempdNM, 3);


			
			if(((MNStruct *)globalcontext)->sysFlags[nTOA] == 4 && nTOA == 317){
			printf("Jitter vals: %i %g %g\n", nTOA, TempdNM[2], sqrt(profnoise));
			for(int j =0; j < Nbins; j++){
				printf("Jitter vals: %i %i %g %g %g %g \n", nTOA, j, TempdNM[2], TempdNM[2]*M[j][2], TempdNM[0]*M[j][0]+TempdNM[1]*M[j][1], (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]);
			}}	
	

			//printf("Jitter vals: %g %g %g %g \n", maxJitter, maxamp, maxJitter/maxamp, TempdNM[2]);
			
		}
		    
		    logMargindet = Margindet;

		    Marginlike = 0;
		    for(int i =0; i < 3; i++){
		    	Marginlike += TempdNM[i]*dNM[i];
		    }

	    profilelike = -0.5*(detN + Chisq + logMargindet + JitterPrior - Marginlike);
	    lnew += profilelike;
	//    if(nTOA == 1){printf("Like: %i  %g %g %g %g \n", nTOA,detN, Chisq, logMargindet, Marginlike);}

	    delete[] shapevec;
	    delete[] Jittervec;
	    delete[] NDiffVec;
	    delete[] dNM;
	    delete[] Betatimes;

	    for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] JitterBasis[j];
		    delete[] HermiteJitterpoly[j];
		    delete[] M[j];
		    delete[] NM[j];
	    }
	    delete[] Hermitepoly;
	    delete[] HermiteJitterpoly;
	    delete[] JitterBasis;
	    delete[] M;
	    delete[] NM;

	    for (int j = 0; j < 3; j++){
		    delete[] MNM[j];
	    }
	    delete[] MNM;
	
	
	 }
	 
	 
	 
	 

	if(badshape == 1){
	//	lnew = -pow(10.0, 10.0);
	}

	//printf("Like: %.10g \n", lnew);
	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;


	return lnew;

}

double  AllTOAStocProfLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){

	//printf("In like \n");

	double *EFAC;
	double *EQUAD;
        long double LDparams[ndim];
        int pcount;

	double uniformpriorterm = 0;


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){

		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

	}	
	
	
	pcount=0;
	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];
	//	printf("timing param %i %.25Lg	\n", p, LDparams[pcount]);
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
           	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
                	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){
                        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
			}
                 }
           }

	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form SATS///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}


        for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
                ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }

	delete[] JumpVec;

	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);      




/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////White noise/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[0]);}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[p]);}
			pcount++;
		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
                }
        }


        double *SQUAD;
	if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
        	SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
        	for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
           		SQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
        	}
    	}	  
	  

	//printf("End of White\n");



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Red Noise///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	double start,end;
	int go=0;
	for (int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++)
	  {
	    if (((MNStruct *)globalcontext)->pulse->obsn[i].deleted==0)
	      {
		if (go==0)
		  {
		    go = 1;
		    start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    end  = start;
		  }
		else
		  {
		    if (start > (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    if (end < (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      end = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		  }
	      }
	  }

	double maxtspan=1*(end-start);


	
	int FitRedCoeff=2*(((MNStruct *)globalcontext)->numFitRedCoeff);
	int totCoeff=0;
	if(((MNStruct *)globalcontext)->incRED != 0)totCoeff+=FitRedCoeff;



	double FreqDet = 0;

	double *freqs = new double[totCoeff];
   	double *SignalCoeff = new double[totCoeff];

	for(int i = 0; i < totCoeff; i++){
		SignalCoeff[i] = 0;
	}	


	double *powercoeff=new double[totCoeff];
	for(int o=0;o<totCoeff; o++){
		powercoeff[o]=0;
	}
	
	double **FMatrix=new double*[((MNStruct *)globalcontext)->pulse->nobs];
	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		FMatrix[i]=new double[totCoeff];
		for(int j=0;j<totCoeff;j++){
			FMatrix[i][j]=0;
		}
	}


	int startpos=0;
	if(((MNStruct *)globalcontext)->incRED==5){

		FreqDet=0;

		for(int i = 0; i < FitRedCoeff; i++){
			SignalCoeff[startpos + i] = Cube[pcount];
			pcount++;
		}
			
		
		for(int pl = 0; pl < ((MNStruct *)globalcontext)->numFitRedPL; pl ++){

			double Tspan = maxtspan;
                        double f1yr = 1.0/3.16e7;


			double redamp=Cube[pcount];
			pcount++;
			double redindex=Cube[pcount];
			pcount++;

			
			redamp=pow(10.0, redamp);
			if(((MNStruct *)globalcontext)->RedPriorType ==1) { uniformpriorterm +=log(redamp); }



			double Agw=redamp;
			for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
				
				freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[i]/maxtspan;
				freqs[startpos+i+FitRedCoeff/2]=freqs[startpos+i];
				
				double rho=0;

 				rho = (Agw*Agw/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-redindex))/(maxtspan*24*60*60);

 				powercoeff[i]+= rho;
 				powercoeff[i+FitRedCoeff/2]+= rho;

				

			}
		}
		
		
		double GWBAmp=0;
		if(((MNStruct *)globalcontext)->incGWB==1){
			GWBAmp=pow(10.0,Cube[pcount]);
			pcount++;

			uniformpriorterm += log(GWBAmp);
			double Tspan = maxtspan;
			double f1yr = 1.0/3.16e7;
			for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
				double rho = (GWBAmp*GWBAmp/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-4.333))/(maxtspan*24*60*60);
				powercoeff[i]+= rho;
				powercoeff[i+FitRedCoeff/2]+= rho;
			}
		}

		for (int i=0; i<FitRedCoeff/2; i++){
			FreqDet=FreqDet+2*log(powercoeff[i]);
		}
		
		for(int i=0;i<FitRedCoeff/2;i++){
			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
				FMatrix[k][i]=cos(2*M_PI*freqs[i]*time);

			}
		}

		for(int i=0;i<FitRedCoeff/2;i++){
			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
				FMatrix[k][i+FitRedCoeff/2]=sin(2*M_PI*freqs[i]*time);
			}
		}

		startpos=FitRedCoeff;

    	}

	double *SignalVec = new double[((MNStruct *)globalcontext)->pulse->nobs];

	
	dgemv(FMatrix,SignalCoeff,SignalVec,((MNStruct *)globalcontext)->pulse->nobs,totCoeff,'N');

	double FreqLike = 0;
	for(int i = 0; i < totCoeff; i++){
		FreqLike += SignalCoeff[i]*SignalCoeff[i]/powercoeff[i];
	}



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	   
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;
	      }
	      
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr  - phase - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
	 }


	int maxshapecoeff = 0;
	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;
	int numshapestoccoeff = ((MNStruct *)globalcontext)->numshapestoccoeff;

	double shapecoeff[numcoeff];
	double StocProfCoeffs[numshapestoccoeff];

	for(int i =0; i < numcoeff; i++){
		if(((MNStruct *)globalcontext)->FixProfile==0){
			shapecoeff[i]= Cube[pcount];
			pcount++;
		}
		else{
			shapecoeff[i]=((MNStruct *)globalcontext)->MeanProfileShape[i];
			//printf("Fixing mean: %i %g n", i, shapecoeff[i]);
		}
	}

	double beta = ((MNStruct *)globalcontext)->MeanProfileBeta*((MNStruct *)globalcontext)->ReferencePeriod;;//Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;

	pcount++;

	for(int i =0; i < numshapestoccoeff; i++){
		StocProfCoeffs[i]= pow(10.0, 2*Cube[pcount]);
		//uniformpriorterm += log(pow(10.0, Cube[pcount]));
		pcount++;
	}

	if(numcoeff+1>=numshapestoccoeff+1){
		maxshapecoeff=numcoeff+1;
	}
	if(numshapestoccoeff+1 > numcoeff+1){
		maxshapecoeff=numshapestoccoeff+1;
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	double lnew = 0;
	int badshape = 0;

	int minoffpulse=0;
	int maxoffpulse=200;
       // int minoffpulse=500;
       // int maxoffpulse=1500;
	
	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
	//	printf("In toa %i \n", t);
		int nTOA = t;

		double detN  = 0;
		double Chisq  = 0;
		double logMargindet = 0;
		double Marginlike = 0;	 
		double JitterPrior = 0;	   

		double profilelike=0;

		long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
		long double FoldingPeriodDays = FoldingPeriod/SECDAY;
		int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
		double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
		double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
		long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

		double *Betatimes = new double[Nbins];
		double **AllHermiteBasis =  new double*[Nbins];
		double **JitterBasis  =  new double*[Nbins];
		double **Hermitepoly =  new double*[Nbins];

		for(int i =0;i<Nbins;i++){AllHermiteBasis[i]=new double[maxshapecoeff];for(int j =0;j<maxshapecoeff;j++){AllHermiteBasis[i][j]=0;}}
		for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}
		for(int i =0;i<Nbins;i++){JitterBasis[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){JitterBasis[i][j]=0;}}
	
	



	

		double sigma=noiseval;
		sigma=pow(10.0, sigma);
//		printf("noise details %i %g %g %g \n", t, noiseval, Tobs, EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]]);

		long double binpos = ModelBats[nTOA];

		if(((MNStruct *)globalcontext)->incRED==5){
			long double rednoiseshift = (long double) SignalVec[t];
			binpos+=rednoiseshift/SECDAY;
			//printf("Red noise shift %i %Lg \n", t, rednoiseshift);
		}

		if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

		long double minpos = binpos - FoldingPeriodDays/2;
		if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];


	    
		for(int j =0; j < Nbins; j++){
			long double timediff = 0;
			long double bintime = ProfileBats[t][j];
			if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
			}
			else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
			}
			else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
			}

			timediff=timediff*SECDAY;
			//if(nTOA == 50){printf("timediff %i %g %g\n", j, (double)timediff, beta);}

			Betatimes[j]=timediff/beta;
			TNothpl(maxshapecoeff,Betatimes[j],AllHermiteBasis[j]);

			for(int k =0; k < maxshapecoeff; k++){
				double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));//*((MNStruct *)globalcontext)->Factorials[k]);
				AllHermiteBasis[j][k]=AllHermiteBasis[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);

				if(k<numcoeff){ Hermitepoly[j][k] = AllHermiteBasis[j][k];}
			
			}

			JitterBasis[j][0] = (1.0/sqrt(2.0))*(-1.0*AllHermiteBasis[j][1]);
			for(int k =1; k < numcoeff; k++){
				JitterBasis[j][k] = (1.0/sqrt(2.0))*(sqrt(double(k))*AllHermiteBasis[j][k-1] - sqrt(double(k+1))*AllHermiteBasis[j][k+1]);
			}	
	    }


	    double *shapevec  = new double[Nbins];
	    double *Jittervec = new double[Nbins];
	
	    double OverallProfileAmp = shapecoeff[0];
	    
	    shapecoeff[0] = 1;

	    dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');
    	    dgemv(JitterBasis,shapecoeff,Jittervec,Nbins,numcoeff,'N');
	
	    double *NDiffVec = new double[Nbins];

	    double maxshape=0;
	    double modelflux=0;
	    double testmodelflux=0;	
            double dataflux = 0;


	    for(int j =0; j < Nbins; j++){

		    detN += log(sigma*sigma);

		    double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] ;//- OverallProfileAmp*shapevec[j];//-offset;

		    if(shapevec[j] < 0)badshape = 1;
		    NDiffVec[j] = datadiff/(sigma*sigma);

		    if(shapevec[j] > maxshape){ maxshape = shapevec[j];}


		    testmodelflux += shapevec[j]*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;

	    }
	    for(int j =0; j < numcoeff; j=j+2){
		//printf("modelflux: %i %g\n", j, ((MNStruct *)globalcontext)->Binomial[j]);
		modelflux+=sqrt(sqrt(M_PI))*sqrt(beta)*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Binomial[j])*shapecoeff[j];
	   }

	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


		double **M = new double*[Nbins];
		double **NM = new double*[Nbins];

		int Msize = 2+numshapestoccoeff;
		int Mcounter=0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
			Msize++;
		}


		for(int i =0; i < Nbins; i++){
			Mcounter=0;
			M[i] = new double[Msize];
			NM[i] = new double[Msize];

			M[i][0] = 1;
			M[i][1] = shapevec[i];

			Mcounter = 2;

			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
//				printf("%i %i %g %g %g \n", nTOA, i, M[i][0], M[i][1], M[i][2]);
				M[i][Mcounter] = Jittervec[i];
				Mcounter++;
			}			



			for(int j = 0; j < numshapestoccoeff; j++){

			    M[i][Mcounter+j] = AllHermiteBasis[i][j];

			}

			for(int j = 0; j < Msize; j++){
				NM[i][j] = M[i][j]/(sigma*sigma);
			//	if(nTOA==50){printf("M %i %i %g %g \n",i,j,M[i][j], sigma); }
			}
		  
		}


		double **MNM = new double*[Msize];
		for(int i =0; i < Msize; i++){
		    MNM[i] = new double[Msize];
		}

		dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');
		//printf("MNM %g %g %g %g \n", MNM[0][0], MNM[0][1], MNM[1][0], MNM[1][1]);

		for(int j = 2; j < Msize; j++){
			//printf("before: %i %g \n", j, MNM[j][j]);
			MNM[j][j] += pow(10.0,20);
		}

		double *dNM = new double[Msize];
		double *TempdNM = new double[Msize];
		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
		}


		int info=0;
		double Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);


		double maxoffset = TempdNM[0];
		double maxamp = TempdNM[1];


		//printf("max %i %g %g %g\n", t, maxoffset, maxamp, Margindet);

		      
		Chisq = 0;
		detN = 0;

		double noisemean=0;
		for(int j =minoffpulse; j < maxoffpulse; j++){
			noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
		}

		noisemean = noisemean/(maxoffpulse-minoffpulse);

		double MLSigma = 0;
		int MLSigmaCount = 0;
		for(int j =0; j < Nbins; j++){
		      double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - maxamp*shapevec[j] - maxoffset;

			res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] -noisemean;
//		      if(shapevec[j] <= maxshape*0.001){MLSigma += res*res; MLSigmaCount += 1;}
		      if(j > minoffpulse && j < maxoffpulse){
				MLSigma += res*res; MLSigmaCount += 1;
		      }
//		      
		}

		MLSigma = sqrt(MLSigma/MLSigmaCount);
		double tempdataflux=0;
		for(int j =0; j < Nbins; j++){
			if(shapevec[j] > maxshape*0.001){
				if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - maxoffset)/MLSigma > 1){
					dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-maxoffset)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
					tempdataflux += ((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-maxoffset);
			//		if(t==1){
                          //                      printf("dataflux %i %g %g %g %g %g\n", j, maxoffset, ((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]), dataflux, tempdataflux, double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24);
                            //            }

				}
			}
		}	

 

//		MLSigma = MLSigma*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];

//		double highfreqnoise = sqrt(double(Nbins)/Tobs)*SQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
		double highfreqnoise = SQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
		double flatnoise = EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];

//		double *testflux=new double[numshapestoccoeff];
//		for(int j = 0; j < numshapestoccoeff; j++){
//			testflux[j] = 0;
//		}


		for(int i =0; i < Nbins; i++){
		  	Mcounter=2;
			double noise = MLSigma*MLSigma +  pow(highfreqnoise*maxamp*shapevec[i],2);

			if(shapevec[i] > maxshape*0.001){
				noise +=  pow(flatnoise*maxamp*maxshape,2);
			}
			//printf("detN: %i %i %g %g %g %g\n", nTOA, i, MLSigma, highfreqnoise, maxamp, noise);
			detN += log(noise);

			double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1];

			if(shapevec[i] < 0)badshape = 1;
			NDiffVec[i] = datadiff/(noise);
		       

			Chisq += datadiff*datadiff/(noise);
	
			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
				//printf("EQUAD: %i %g %g %g %g \n", i, dataflux, modelflux, beta, M[i][2]);
				M[i][2] = M[i][2]*dataflux/modelflux/beta;
				Mcounter++;
			}
			
		   	for(int j = 0; j < numshapestoccoeff; j++){
//				testflux[j] += pow(M[i][Mcounter+j]*sqrt(double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24)*dataflux, 2);
//				M[i][Mcounter+j] = M[i][Mcounter+j]*dataflux*sqrt(Tobs/double(Nbins));
				M[i][Mcounter+j] = M[i][Mcounter+j]*dataflux;

		    	}


			for(int j = 0; j < Msize; j++){
				NM[i][j] = M[i][j]/(noise);
			}


		}
		
//		for(int j = 0; j < numshapestoccoeff; j++){
//			printf("test flux %i %i %g %g \n", t, j, testflux[j], pow(dataflux,2));
//		}
//		delete[] testflux;
			
		dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');


		Mcounter=2;
		double JitterDet = 0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
			double profnoise = EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];//sqrt(double(Nbins)/Tobs)*EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
			profnoise = profnoise*profnoise;
		    	JitterDet = log(profnoise);
			//printf("after: %i %g %g\n", Mcounter, MNM[2][2], 1.0/profnoise);
		    	MNM[2][2] += 1.0/profnoise;
			Mcounter++;
		}


		double StocProfDet = 0;
		for(int j = 0; j < numshapestoccoeff; j++){
			if(((MNStruct *)globalcontext)->pulse->obsn[nTOA].sat > 57075.0){
			StocProfDet += log(StocProfCoeffs[j]);
			MNM[Mcounter+j][Mcounter+j] +=  1.0/StocProfCoeffs[j];
			}
			else{
                        StocProfDet += log(pow(10.0, -20));
                        MNM[Mcounter+j][Mcounter+j] +=  1.0/pow(10.0, -20);
			}
		}

		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
		}


		info=0;
		Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);



//		if(t==1){
//			double *tempsignal = new double[Nbins];
//			dgemv(M,TempdNM,tempsignal,Nbins,Msize,'N');
	//		for(int j = 0; j < numshapestoccoeff; j++){
	//			double stocflux = 0;
	//			for(int k = 0; k < Nbins; k++){
	//				stocflux += pow(TempdNM[2+j]*M[k][2+j],2);
	//			}
	//			printf("%i %g %g %g %g %g\n", j, StocProfCoeffs[j], TempdNM[2+j], stocflux, dataflux*dataflux/(double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24), double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24);
	//		}
			
			
//			for(int j = 0; j < Nbins; j++){
				//double justprofile = TempdNM[0]*M[j][0] + TempdNM[1]*M[j][1];
//				printf("%i %g %g\n", j,(double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1],tempsignal[j]); 
				//printf("%i %g %g %g %g %g %g %g %g %g %g %g %g \n",j, TempdNM[2]*M[j][2], TempdNM[3]*M[j][3], TempdNM[4]*M[j][4], TempdNM[5]*M[j][5], TempdNM[6]*M[j][6], TempdNM[7]*M[j][7], TempdNM[8]*M[j][8], TempdNM[9]*M[j][9], TempdNM[10]*M[j][10],TempdNM[11]*M[j][11],justprofile, (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] );
//			}
//			delete[] tempsignal;
//		}			
		
			    
		logMargindet = Margindet;

		Marginlike = 0;
		for(int i =0; i < Msize; i++){
			Marginlike += TempdNM[i]*dNM[i];
		}

		profilelike = -0.5*(detN + Chisq + logMargindet + StocProfDet + JitterDet - Marginlike);
		lnew += profilelike;
//		printf("Like: %i %.15g %.15g %.15g %.15g %.15g %.15g %.15g\n", nTOA,lnew, detN, Chisq, logMargindet, Marginlike, StocProfDet , JitterDet);

		delete[] shapevec;
		delete[] Jittervec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] Betatimes;

		for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] JitterBasis[j];
		    delete[] AllHermiteBasis[j];
		    delete[] M[j];
		    delete[] NM[j];
		}
		delete[] Hermitepoly;
		delete[] AllHermiteBasis;
		delete[] JitterBasis;
		delete[] M;
		delete[] NM;

		for (int j = 0; j < Msize; j++){
		    delete[] MNM[j];
		}
		delete[] MNM;
	
	
	}
	 
	 
	 
	 

	if(badshape == 1){
	//	lnew = -pow(10.0, 10.0);
	}

	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;
	delete[] powercoeff;
	delete[] freqs;
	delete[] SignalCoeff;
	delete[] SignalVec;
	
	for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
		delete[]FMatrix[j];
	}
	delete[]FMatrix;


	lnew += -0.5*(FreqDet + FreqLike) + uniformpriorterm;
	//printf("End Like: %.10g %.10g %.10g %.10g\n", lnew, FreqDet, FreqLike, uniformpriorterm);

	return lnew;

}


double  AllTOALike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){



	double *EFAC;
	double *EQUAD;
	//printf("In one toa like\n");
        long double LDparams[ndim];
        int pcount;

//	for(int i = 0; i < ndim; i++){
//		printf("Cube: %i %g \n", i, Cube[i]);
//	}
	
	
	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){


		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

		//printf("LDparams: %i %.15Lg %.15Lg \n", p,((MNStruct *)globalcontext)->LDpriors[p][0], ((MNStruct *)globalcontext)->LDpriors[p][1]); 


	}	
	
	
	pcount=0;
	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	//printf("Phase %.15Lg,  %.15Lg,  %.15Lg \n", phase, LDparams[0], ((MNStruct *)globalcontext)->ReferencePeriod/SECDAY);
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];	
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}


	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
           	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
                	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){
                        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
			}
                 }
           }

	}


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}


        for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
                ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }

	delete[] JumpVec;



	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);       
	  
	  
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		//if(p==0){printf("after: %.25Lg %.25Lg %.15g \n", ((MNStruct *)globalcontext)->pulse->obsn[p].sat, ((MNStruct *)globalcontext)->pulse->obsn[p].bat, (double)((MNStruct *)globalcontext)->pulse->obsn[p].residual);}
		//printf("sat: %.25Lg %.25Lg %.25Lg \n", ((MNStruct *)globalcontext)->pulse->obsn[p].origsat, ((MNStruct *)globalcontext)->pulse->obsn[p].sat, phase);
	}
	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=1;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			pcount++;
		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
                }
        }
	  
	  
	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	   
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      //long double sat2bat = ((MNStruct *)globalcontext)->pulse->obsn[i].bat - ((MNStruct *)globalcontext)->pulse->obsn[i].sat;
	      //printf("Sat2Bat %i %.25Lg  %.25Lg \n", i, ((MNStruct *)globalcontext)->pulse->obsn[i].sat, sat2bat/SECDAY);
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;
// 		    printf("Sat2Bat %i %i %.25Lg %.25Lg %.25Lg \n", i, j, sat2bat/SECDAY, ((MNStruct *)globalcontext)->ProfileData[i][j][0], ProfileBats[i][j]);
	      }
	      
	      //ModelBats[i] = ((MNStruct *)globalcontext)->pulse->obsn[i].bat - phase ;//- ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY - phase;
	      //printf("Bats %i %.20Lg %.20Lg %.20Lg %.20Lg \n", i,ModelBats[i],((MNStruct *)globalcontext)->ProfileInfo[i][5], ((MNStruct *)globalcontext)->pulse->obsn[i].residual, ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr);
	     // printf("Sat2BatConmp %.10Lg %.10Lg \n",((MNStruct *)globalcontext)->ProfileInfo[i][6], ((MNStruct *)globalcontext)->pulse->obsn[i].bat);
	 }


	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;

	double shapecoeff[numcoeff];

	for(int i =0; i < numcoeff; i++){

		shapecoeff[i]= Cube[pcount];
		Cube[pcount] = shapecoeff[i];
		pcount++;
	}
	//printf("Beta: %g %.25Lg \n", Cube[pcount], ((MNStruct *)globalcontext)->ReferencePeriod);
	double beta = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;
	pcount++;

	double lnew = 0;
	int badshape = 0;
	
	 for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
	    int nTOA = t;
	    
	    double detN  = 0;
	    double Chisq  = 0;
	    double logMargindet = 0;
	    double Marginlike = 0;	 
	    
	    double profilelike=0;
	    
	    long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
	    long double FoldingPeriodDays = FoldingPeriod/SECDAY;
	    int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
	    double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
	    double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
	    long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

	    double *Betatimes = new double[Nbins];
	    double **Hermitepoly =  new double*[Nbins];
	    for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}
	
	



	

	    double sigma=noiseval;
	    sigma=pow(10.0, sigma)*sqrt(double(Nbins)/Tobs)*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];

	    //printf("CheckBat %i  %.20Lg %.20Lg %.20Lg  \n", nTOA, ProfileBats[nTOA][0], ProfileBats[nTOA][Nbins-1],((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat);
	    //printf("CheckSat %i  %.20Lg %.20Lg %.20Lg  \n", nTOA,((MNStruct *)globalcontext)->ProfileData[nTOA][0][0], ((MNStruct *)globalcontext)->ProfileData[nTOA][Nbins-1][0],((MNStruct *)globalcontext)->pulse->obsn[nTOA].sat );
	    long double binpos = ModelBats[nTOA];
	    if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;
	    long double minpos = binpos - FoldingPeriodDays/2;
	    //if(t==120){printf("minpos maxpos %i %.25Lg %.25Lg %.25Lg %.25Lg %.25Lg\n", t, ProfileBats[nTOA][0], ProfileBats[nTOA][Nbins-1], binpos, minpos, binpos + FoldingPeriodDays/2);}
	    if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
	    long double maxpos = binpos + FoldingPeriodDays/2;
	    if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];

	    //printf("Obvious time check: %.25Lg %.25Lg %.25Lg %.25Lg \n", ((MNStruct *)globalcontext)->ProfileData[nTOA][0][0], FoldingPeriodDays, ((MNStruct *)globalcontext)->ProfileData[nTOA][0][0]+FoldingPeriodDays, ((MNStruct *)globalcontext)->ProfileData[nTOA][Nbins-1][0]); 

	    for(int j =0; j < Nbins; j++){
		    long double timediff = 0;
		    long double bintime = ProfileBats[t][j];
		    if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
		    }
		    else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
		    }
		    else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
		    }

		    timediff=timediff*SECDAY;
	//	    timediff = timediff - 1.6674E-7 + 6.8229E-8 + 7.8479E-8;
	//	    if(t==120){printf("Bins: %i %i %.20Lg %.20Lg %.20Lg %.20Lg %.20Lg  \n", nTOA, j, minpos, maxpos, binpos, bintime, timediff);}
			if(j == 0){
                    //            printf("Model Bat: %i %s %g %g \n", t, ((MNStruct *)globalcontext)->pulse->obsn[t].fname , (double)ProfileBats[t][j], (double)timediff);
                        }


		    Betatimes[j]=timediff/beta;
		    TNothpl(numcoeff,Betatimes[j],Hermitepoly[j]);
		    for(int k =0; k < numcoeff; k++){
			    double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI)*((MNStruct *)globalcontext)->Factorials[k]);
			
		//		printf("%i %i  %i %g %g %g %.25Lg %g\n", t, j, k, Hermitepoly[j][k], Bconst, Betatimes[j], timediff, beta);

			    Hermitepoly[j][k]=Hermitepoly[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);
		    }
	    }


	    double *shapevec  = new double[Nbins];
	    double OverallProfileAmp = shapecoeff[0];
	    
	    shapecoeff[0] = 1;

	    dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');


	
	    double *NDiffVec = new double[Nbins];

	    double maxshape=0;
	    for(int j =0; j < Nbins; j++){

		    detN += log(sigma*sigma);

		    double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] ;//- OverallProfileAmp*shapevec[j];//-offset;

		    if(shapevec[j] < 0)badshape = 1;
		    NDiffVec[j] = datadiff/(sigma*sigma);
	            //printf("%i %i %g  %g \n", t, j, datadiff, shapevec[j]);
		    Chisq += datadiff*datadiff/(sigma*sigma);
		    if(shapevec[j] > maxshape){ maxshape = shapevec[j];}
	    }

	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////
	

	    double **M = new double*[Nbins];
	    double **NM = new double*[Nbins];

	    for(int i =0; i < Nbins; i++){
		    M[i] = new double[2];
		    NM[i] = new double[2];
		    
		    M[i][0] = 1;
		    M[i][1] = shapevec[i];

		    NM[i][0] = M[i][0]/(sigma*sigma);
		    NM[i][1] = M[i][1]/(sigma*sigma);
	    }


	    double **MNM = new double*[2];
	    for(int i =0; i < 2; i++){
		    MNM[i] = new double[2];
	    }

	    dgemm(M, NM , MNM, Nbins, 2,Nbins, 2, 'T', 'N');

	    double *dNM = new double[2];
	    dNM[0] = 0;
	    dNM[1] = 0;

	    for(int j =0; j < Nbins; j++){
		    dNM[0] += NDiffVec[j]*M[j][0];
		    dNM[1] += NDiffVec[j]*M[j][1];
	    }
	    
	    double Margindet = MNM[0][0]*MNM[1][1] - MNM[0][1]*MNM[1][0];

            double maxamp = (1.0/Margindet)*(-dNM[0]*MNM[0][1] + dNM[1]*MNM[0][0]);
            double maxoffset = (1.0/Margindet)*(dNM[0]*MNM[1][1] - dNM[1]*MNM[0][1]);
	    
//	    if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
	      
	      
		Chisq = 0;
		detN = 0;
	      
		double profnoise = sqrt(double(Nbins)/Tobs)*EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
		//printf("%i %g %g \n", nTOA, sigma, profnoise*maxamp);
		
		double MLSigma = 0;
		int MLSigmaCount = 0;
		for(int j =0; j < Nbins; j++){
		      double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - maxamp*shapevec[j] - maxoffset;
		      if(shapevec[j] <= maxshape*0.01){MLSigma += res*res; MLSigmaCount += 1;}
//			if(t==120){printf("res: %i %i %g %g %g %g \n", t, j, (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1],  maxamp*shapevec[j], maxoffset, maxamp*shapevec[j] + maxoffset);}
		}
		MLSigma=sqrt(MLSigma/MLSigmaCount)*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];
	        //MLSigma=0.1;	
		for(int j =0; j < Nbins; j++){
		  
			double noise = MLSigma*MLSigma + pow(profnoise*maxamp*shapevec[j],2);
		
			detN += log(noise);

			double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] ;//- OverallProfileAmp*shapevec[j];//-offset;

			if(shapevec[j] < 0)badshape = 1;
			NDiffVec[j] = datadiff/(noise);
//		        printf("%i %i %g %g\n", t, j, maxamp*shapevec[j] + maxoffset, datadiff);
			Chisq += datadiff*datadiff/(noise);
			
			 NM[j][0] = M[j][0]/(noise);
			 NM[j][1] = M[j][1]/(noise);

		
		}
		
		dgemm(M, NM , MNM, Nbins, 2,Nbins, 2, 'T', 'N');
		
		dNM[0] = 0;
		dNM[1] = 0;

		for(int j =0; j < Nbins; j++){
			dNM[0] += NDiffVec[j]*M[j][0];
			dNM[1] += NDiffVec[j]*M[j][1];

		}
		
		Margindet = MNM[0][0]*MNM[1][1] - MNM[0][1]*MNM[1][0];
		
	    maxamp = (1.0/Margindet)*(-dNM[0]*MNM[0][1] + dNM[1]*MNM[0][0]);
            maxoffset = (1.0/Margindet)*(dNM[0]*MNM[1][1] - dNM[1]*MNM[0][1]);

	                for(int j =0; j < Nbins; j++){

                        if(((MNStruct *)globalcontext)->sysFlags[nTOA] == 0){
                                //printf("%i %i %g %g %g %g %g\n", nTOA, j, (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1], maxoffset+maxamp*shapevec[j], MLSigma, profnoise*maxamp*shapevec[j], sqrt(MLSigma*MLSigma + pow(profnoise*maxamp*shapevec[j],2)));
                        }
                }

	
//	    }
	    
	    logMargindet += log(Margindet);

	    Marginlike += (1.0/Margindet)*(dNM[0]*MNM[1][1]*dNM[0] + dNM[1]*MNM[0][0]*dNM[1] - 2*dNM[0]*MNM[0][1]*dNM[1]);

	    profilelike = -0.5*(detN + Chisq + logMargindet - Marginlike);
	    lnew += profilelike;
	///	printf("profile like %i %g %g \n", t, profilelike, lnew);
	    //if(t == 1){printf("Like: %i  %g %g %g %g \n", nTOA,detN, Chisq, logMargindet, Marginlike);}

	    delete[] shapevec;
	    delete[] NDiffVec;
	    delete[] dNM;
	    delete[] Betatimes;

	    for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] M[j];
		    delete[] NM[j];
	    }
	    delete[] Hermitepoly;
	    delete[] M;
	    delete[] NM;

	    for (int j = 0; j < 2; j++){
		    delete[] MNM[j];
	    }
	    delete[] MNM;
	
	
	 }
	 
	 
	 
	 

	if(badshape == 1){
	//	lnew = -pow(10.0, 10.0);
	}
	if(isnan(lnew)){
		lnew= -pow(10.0, 10.0);
	}
	//printf("Like: %.10g \n", lnew);
	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;


	return lnew;

}


double  AllTOASim(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){



	double *EFAC;
	double *EQUAD;
	//printf("In one toa like\n");
        long double LDparams[ndim];
        int pcount;

//	for(int i = 0; i < ndim; i++){
//		printf("Cube: %i %g \n", i, Cube[i]);
//	}
	
	
	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){


		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

		printf("LDparams: %i %.15Lg \n", p, LDparams[p]); 


	}	
	
	
	pcount=0;
	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	//printf("Phase %.15Lg,  %.15Lg,  %.15Lg \n", phase, LDparams[0], ((MNStruct *)globalcontext)->ReferencePeriod/SECDAY);
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];	
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}





	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
           	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
                	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){
                        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
			}
                 }
           }

	}


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}


        for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
                ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }

	delete[] JumpVec;



	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);       
	  
	  
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		//if(p==0){printf("after: %.25Lg %.25Lg %.15g \n", ((MNStruct *)globalcontext)->pulse->obsn[p].sat, ((MNStruct *)globalcontext)->pulse->obsn[p].bat, (double)((MNStruct *)globalcontext)->pulse->obsn[p].residual);}
		//printf("sat: %.25Lg %.25Lg %.25Lg \n", ((MNStruct *)globalcontext)->pulse->obsn[p].origsat, ((MNStruct *)globalcontext)->pulse->obsn[p].sat, phase);
	}
	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=1;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			pcount++;
		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
                }
        }
	  
	  
	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	   
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      //long double sat2bat = ((MNStruct *)globalcontext)->pulse->obsn[i].bat - ((MNStruct *)globalcontext)->pulse->obsn[i].sat;
	      //printf("Sat2Bat %i %.25Lg  %.25Lg \n", i, ((MNStruct *)globalcontext)->pulse->obsn[i].sat, sat2bat/SECDAY);
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;
// 		    printf("Sat2Bat %i %i %.25Lg %.25Lg %.25Lg \n", i, j, sat2bat/SECDAY, ((MNStruct *)globalcontext)->ProfileData[i][j][0], ProfileBats[i][j]);
	      }
	      
	      //ModelBats[i] = ((MNStruct *)globalcontext)->pulse->obsn[i].bat - phase ;//- ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY - phase;
	      //printf("Bats %i %.20Lg %.20Lg %.20Lg %.20Lg \n", i,ModelBats[i],((MNStruct *)globalcontext)->ProfileInfo[i][5], ((MNStruct *)globalcontext)->pulse->obsn[i].residual, ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr);
	     // printf("Sat2BatConmp %.10Lg %.10Lg \n",((MNStruct *)globalcontext)->ProfileInfo[i][6], ((MNStruct *)globalcontext)->pulse->obsn[i].bat);
	 }


	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;

	double shapecoeff[numcoeff];

	for(int i =0; i < numcoeff; i++){

		shapecoeff[i]= Cube[pcount];
		Cube[pcount] = shapecoeff[i];
		pcount++;
	}
	//printf("Beta: %g %.25Lg \n", Cube[pcount], ((MNStruct *)globalcontext)->ReferencePeriod);
	double beta = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;
	pcount++;

	double lnew = 0;
	int badshape = 0;
	
	 for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){

		int nTOA = t;

		double detN  = 0;
		double Chisq  = 0;
		double logMargindet = 0;
		double Marginlike = 0;	 

		double profilelike=0;

		long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
		long double FoldingPeriodDays = FoldingPeriod/SECDAY;
		int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
		double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
		double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
		long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

		double *Betatimes = new double[Nbins];
		double **Hermitepoly =  new double*[Nbins];
		for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}

	



	

		double sigma=noiseval;
		sigma=pow(10.0, sigma)*sqrt(double(Nbins)/Tobs)*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];

		//printf("CheckBat %i  %.20Lg %.20Lg %.20Lg  \n", nTOA, ProfileBats[nTOA][0], ProfileBats[nTOA][Nbins-1],((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat);
		//printf("CheckSat %i  %.20Lg %.20Lg %.20Lg  \n", nTOA,((MNStruct *)globalcontext)->ProfileData[nTOA][0][0], ((MNStruct *)globalcontext)->ProfileData[nTOA][Nbins-1][0],((MNStruct *)globalcontext)->pulse->obsn[nTOA].sat );
		long double binpos = ModelBats[nTOA];



		long equadseed = 1000;
                long double equadnoiseval = (long double)TKgaussDev(&equadseed)*pow(10.0, -6.3);
		//if(((MNStruct *)globalcontext)->sysFlags[nTOA] == 4){printf("EQUAD: %i %.25Lg %.25Lg %.25Lg %.25Lg \n",t,equadnoiseval/SECDAY,equadnoiseval, binpos, binpos+equadnoiseval/SECDAY);}
	        //if(((MNStruct *)globalcontext)->sysFlags[nTOA] == 4){binpos+=equadnoiseval/SECDAY;}



		if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;
		long double minpos = binpos - FoldingPeriodDays/2;
		//if(t==120){printf("minpos maxpos %i %.25Lg %.25Lg %.25Lg %.25Lg %.25Lg\n", t, ProfileBats[nTOA][0], ProfileBats[nTOA][Nbins-1], binpos, minpos, binpos + FoldingPeriodDays/2);}
		if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];

		//printf("Obvious time check: %.25Lg %.25Lg %.25Lg %.25Lg \n", ((MNStruct *)globalcontext)->ProfileData[nTOA][0][0], FoldingPeriodDays, ((MNStruct *)globalcontext)->ProfileData[nTOA][0][0]+FoldingPeriodDays, ((MNStruct *)globalcontext)->ProfileData[nTOA][Nbins-1][0]); 


		for(int j =0; j < Nbins; j++){

		    long double timediff = 0;
		    long double bintime = ProfileBats[t][j];

		    
		   
		    if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
		    }
		    else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
		    }
		    else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
		    }

		    timediff=timediff*SECDAY;
	//	    timediff = timediff - 1.6674E-7 + 6.8229E-8 + 7.8479E-8;
	//	    if(t==120){printf("Bins: %i %i %.20Lg %.20Lg %.20Lg %.20Lg %.20Lg  \n", nTOA, j, minpos, maxpos, binpos, bintime, timediff);}
			if(j == 0){
                    //            printf("Model Bat: %i %s %g %g \n", t, ((MNStruct *)globalcontext)->pulse->obsn[t].fname , (double)ProfileBats[t][j], (double)timediff);
                        }


		    Betatimes[j]=timediff/beta;
		    TNothpl(numcoeff,Betatimes[j],Hermitepoly[j]);
		    for(int k =0; k < numcoeff; k++){
			    double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI)*((MNStruct *)globalcontext)->Factorials[k]);
			
		//		printf("%i %i  %i %g %g %g %.25Lg %g\n", t, j, k, Hermitepoly[j][k], Bconst, Betatimes[j], timediff, beta);

			    Hermitepoly[j][k]=Hermitepoly[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);
		    }
		}


	    double *shapevec  = new double[Nbins];
	    double OverallProfileAmp = shapecoeff[0];
	    
	    shapecoeff[0] = 1;

	    dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');


	
	    double *NDiffVec = new double[Nbins];

	    double maxshape=0;
	    for(int j =0; j < Nbins; j++){

		    detN += log(sigma*sigma);

		    double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] ;//- OverallProfileAmp*shapevec[j];//-offset;

		    if(shapevec[j] < 0)badshape = 1;
		    NDiffVec[j] = datadiff/(sigma*sigma);
	            //printf("%i %i %g  %g \n", t, j, datadiff, shapevec[j]);
		    Chisq += datadiff*datadiff/(sigma*sigma);
		    if(shapevec[j] > maxshape){ maxshape = shapevec[j];}
	    }

	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////
	

	    double **M = new double*[Nbins];
	    double **NM = new double*[Nbins];

	    for(int i =0; i < Nbins; i++){
		    M[i] = new double[2];
		    NM[i] = new double[2];
		    
		    M[i][0] = 1;
		    M[i][1] = shapevec[i];

		    NM[i][0] = M[i][0]/(sigma*sigma);
		    NM[i][1] = M[i][1]/(sigma*sigma);
	    }


		double **MNM = new double*[2];
		for(int i =0; i < 2; i++){
		    MNM[i] = new double[2];
		}

		dgemm(M, NM , MNM, Nbins, 2,Nbins, 2, 'T', 'N');

		double *dNM = new double[2];
		dNM[0] = 0;
		dNM[1] = 0;

		for(int j =0; j < Nbins; j++){
		    dNM[0] += NDiffVec[j]*M[j][0];
		    dNM[1] += NDiffVec[j]*M[j][1];
		}

		double Margindet = MNM[0][0]*MNM[1][1] - MNM[0][1]*MNM[1][0];

		double maxamp = (1.0/Margindet)*(-dNM[0]*MNM[0][1] + dNM[1]*MNM[0][0]);
		double maxoffset = (1.0/Margindet)*(dNM[0]*MNM[1][1] - dNM[1]*MNM[0][1]);

		double MLSigma = 0;
		int MLSigmaCount = 0;
		for(int j =0; j < Nbins; j++){
		      double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - maxamp*shapevec[j] - maxoffset;
		      if(shapevec[j] <= maxshape*0.01){MLSigma += res*res; MLSigmaCount += 1;}
		}

		MLSigma = sqrt(MLSigma/MLSigmaCount);

		double dataflux = 0;
		for(int j =0; j < Nbins; j++){
			if(shapevec[j] > maxshape*0.001){
				if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - maxoffset)/MLSigma > 1){
					dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-maxoffset)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
				}
			}
		}	


		std::string ProfileName =  ((MNStruct *)globalcontext)->pulse->obsn[t].fname;
		std::string fname = "/home/ltl21/scratch/Pulsars/ProfileData/J1909-10cm/profiles/"+ProfileName+".ASCII";
		std::ifstream ProfileFile;
                                
		ProfileFile.open(fname.c_str());
		

		std::string line; 
		getline(ProfileFile,line);
		
		ProfileFile.close();


		std::string fname2 = "/home/ltl21/scratch/Pulsars/ProfileData/J1909-10cm/SimNoNoiseProfiles/"+ProfileName+".ASCII";
	        std::ofstream simfile;
		simfile.open(fname2.c_str());
		simfile << line << "\n";
		for(int i = 0; i < Nbins; i++){
			long seed = 1000;
                        double noiseval = TKgaussDev(&seed)*MLSigma;
			double gaussflux = sqrt(sqrt(M_PI))*sqrt(beta)*pow(2.0, 0.5*(1.0));
			//printf("%i %i %g %g %g \n", t, i, dataflux, gaussflux, Hermitepoly[i][0]);
			simfile << i << " " <<  std::fixed  << std::setprecision(8) << (dataflux/gaussflux)*Hermitepoly[i][0] << "\n";
		}

		simfile.close();

		delete[] shapevec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] Betatimes;

		for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] M[j];
		    delete[] NM[j];
		}
		delete[] Hermitepoly;
		delete[] M;
		delete[] NM;

		for (int j = 0; j < 2; j++){
		    delete[] MNM[j];
		}
		delete[] MNM;
	
	
	 }
	 
	 
	 
	printf("made sim \n");
	//printf("Like: %.10g \n", lnew);
	usleep(5000000);
	if(badshape == 1){
	//	lnew = -pow(10.0, 10.0);
	}

	
	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;


	return lnew;

}


double  AllTOAMaxLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){



	double *EFAC;
	double *EQUAD;
	//printf("In one toa like\n");
        long double LDparams[ndim];
        int pcount;

//	for(int i = 0; i < ndim; i++){
//		printf("Cube: %i %g \n", i, Cube[i]);
//	}
	
	
	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){


		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

		//printf("LDparams: %i %.15Lg %.15Lg \n", p,((MNStruct *)globalcontext)->LDpriors[p][0], ((MNStruct *)globalcontext)->LDpriors[p][1]); 


	}	
	
	
	pcount=0;
	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	//printf("Phase %.15Lg,  %.15Lg,  %.15Lg \n", phase, LDparams[0], ((MNStruct *)globalcontext)->ReferencePeriod/SECDAY);
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];	
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}


	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
           	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
                	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){
                        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
			}
                 }
           }

	}


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}


        for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
                ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }

	delete[] JumpVec;



	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);      
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);     
	  
	  
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		//if(p==0){printf("after: %.25Lg %.25Lg %.15g \n", ((MNStruct *)globalcontext)->pulse->obsn[p].sat, ((MNStruct *)globalcontext)->pulse->obsn[p].bat, (double)((MNStruct *)globalcontext)->pulse->obsn[p].residual);}
		//printf("sat: %.25Lg %.25Lg %.25Lg \n", ((MNStruct *)globalcontext)->pulse->obsn[p].origsat, ((MNStruct *)globalcontext)->pulse->obsn[p].sat, phase);
	}
	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=1;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			pcount++;
		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
                }
        }
	  
	  
	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	   
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      //long double sat2bat = ((MNStruct *)globalcontext)->pulse->obsn[i].bat - ((MNStruct *)globalcontext)->pulse->obsn[i].sat;
	      //printf("Sat2Bat %i %.25Lg  %.25Lg \n", i, ((MNStruct *)globalcontext)->pulse->obsn[i].sat, sat2bat/SECDAY);
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;
// 		    printf("Sat2Bat %i %i %.25Lg %.25Lg %.25Lg \n", i, j, sat2bat/SECDAY, ((MNStruct *)globalcontext)->ProfileData[i][j][0], ProfileBats[i][j]);
	      }
	      
	      //ModelBats[i] = ((MNStruct *)globalcontext)->pulse->obsn[i].bat - phase ;//- ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY - phase;
	      //printf("Bats %i %.20Lg %.20Lg %.20Lg %.20Lg \n", i,ModelBats[i],((MNStruct *)globalcontext)->ProfileInfo[i][5], ((MNStruct *)globalcontext)->pulse->obsn[i].residual, ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr);
	     // printf("Sat2BatConmp %.10Lg %.10Lg \n",((MNStruct *)globalcontext)->ProfileInfo[i][6], ((MNStruct *)globalcontext)->pulse->obsn[i].bat);
	 }


	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;

	double shapecoeff[numcoeff];

	for(int i =0; i < numcoeff; i++){

		shapecoeff[i]= Cube[pcount];
		Cube[pcount] = shapecoeff[i];
		pcount++;
	}
	//printf("Beta: %g %.25Lg \n", Cube[pcount], ((MNStruct *)globalcontext)->ReferencePeriod);
	double beta = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;
	pcount++;

	double lnew = 0;
	int badshape = 0;
	
	 for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
	    int nTOA = t;
	    
	    double detN  = 0;
	    double Chisq  = 0;
	    double logMargindet = 0;
	    double Marginlike = 0;	 
	    
	    double profilelike=0;
	    
	    long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
	    long double FoldingPeriodDays = FoldingPeriod/SECDAY;
	    int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
	    double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
	    double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
	    long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

	    double *Betatimes = new double[Nbins];
	    double **Hermitepoly =  new double*[Nbins];
	    for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}
	
	



	

	    double sigma=noiseval;
	    sigma=pow(10.0, sigma)*sqrt(double(Nbins)/Tobs)*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];

	    //printf("CheckBat %i  %.20Lg %.20Lg %.20Lg  \n", nTOA, ProfileBats[nTOA][0], ProfileBats[nTOA][Nbins-1],((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat);
	    //printf("CheckSat %i  %.20Lg %.20Lg %.20Lg  \n", nTOA,((MNStruct *)globalcontext)->ProfileData[nTOA][0][0], ((MNStruct *)globalcontext)->ProfileData[nTOA][Nbins-1][0],((MNStruct *)globalcontext)->pulse->obsn[nTOA].sat );
	    long double binpos = ModelBats[nTOA];
	    if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;
	    long double minpos = binpos - FoldingPeriodDays/2;
	    //if(t==120){printf("minpos maxpos %i %.25Lg %.25Lg %.25Lg %.25Lg %.25Lg\n", t, ProfileBats[nTOA][0], ProfileBats[nTOA][Nbins-1], binpos, minpos, binpos + FoldingPeriodDays/2);}
	    if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
	    long double maxpos = binpos + FoldingPeriodDays/2;
	    if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];

	    //printf("Obvious time check: %.25Lg %.25Lg %.25Lg %.25Lg \n", ((MNStruct *)globalcontext)->ProfileData[nTOA][0][0], FoldingPeriodDays, ((MNStruct *)globalcontext)->ProfileData[nTOA][0][0]+FoldingPeriodDays, ((MNStruct *)globalcontext)->ProfileData[nTOA][Nbins-1][0]); 

	    for(int j =0; j < Nbins; j++){
		    long double timediff = 0;
		    long double bintime = ProfileBats[t][j];
		    if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
		    }
		    else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
		    }
		    else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
		    }

		    timediff=timediff*SECDAY;
	//	    timediff = timediff - 1.6674E-7 + 6.8229E-8 + 7.8479E-8;
	//	    if(t==120){printf("Bins: %i %i %.20Lg %.20Lg %.20Lg %.20Lg %.20Lg  \n", nTOA, j, minpos, maxpos, binpos, bintime, timediff);}
			if(j == 0){
                    //            printf("Model Bat: %i %s %g %g \n", t, ((MNStruct *)globalcontext)->pulse->obsn[t].fname , (double)ProfileBats[t][j], (double)timediff);
                        }


		    Betatimes[j]=timediff/beta;
		    TNothpl(numcoeff,Betatimes[j],Hermitepoly[j]);
		    for(int k =0; k < numcoeff; k++){
			    double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI)*((MNStruct *)globalcontext)->Factorials[k]);
			
				//printf("%i %i  %i %g %g %g %.25Lg %g %g\n", t, j, k, Hermitepoly[j][k], Bconst, Betatimes[j], timediff, beta,Hermitepoly[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]));

			    Hermitepoly[j][k]=Hermitepoly[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);
		    }
	    }


	    double *shapevec  = new double[Nbins];
	    double OverallProfileAmp = shapecoeff[0];
	    
	    shapecoeff[0] = 1;

	    dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');


		sigma=1;
	    double *NDiffVec = new double[Nbins];

	    double maxshape=0;
	    for(int j =0; j < Nbins; j++){

		    detN += log(sigma*sigma);

		    double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] ;//- OverallProfileAmp*shapevec[j];//-offset;

		    if(shapevec[j] < 0)badshape = 1;
		    NDiffVec[j] = datadiff/(sigma*sigma);
	            //printf("%i %i %g  %g \n", t, j, datadiff, shapevec[j]);
		    Chisq += datadiff*datadiff/(sigma*sigma);
		    if(shapevec[j] > maxshape){ maxshape = shapevec[j];}
	    }

	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////
	
		
		double **M = new double*[Nbins];
		double **NM = new double*[Nbins];

		int Msize = 1 + numcoeff;

		for(int i =0; i < Nbins; i++){
		    M[i] = new double[Msize];
		    NM[i] = new double[Msize];
		    
		    M[i][0] = 1;

		    for(int k =0; k < numcoeff; k++){
			
			M[i][1+k] = Hermitepoly[i][k];
		    }

		    for(int k =0; k < Msize; k++){
		   	 NM[i][k] = M[i][k]/(sigma*sigma);
		    }

		}


		double **MNM = new double*[Msize];
		for(int i =0; i < Msize; i++){
		    MNM[i] = new double[Msize];
		}

		dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');

		for(int k =0; k < numcoeff; k++){
			for(int i =0; i < Nbins; i++){
				//printf("H: %i %i %g \n", i, k, Hermitepoly[i][k]);
			}
		}
		for(int i =0; i < Msize; i++){
			for(int j =0; j < Msize; j++){
				//printf("M: %i %i %g \n", i, j, MNM[i][j]);
			}
		}

		double *dNM = new double[Msize];
		double *TempdNM = new double[Msize];


		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
			MNM[i][i] += pow(10.0, -12)*MNM[i][i];
		}

		


		int info=0;
		double Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);


		

		dgemv(M,TempdNM,shapevec,Nbins,Msize,'N');
			    

	      
	      
		Chisq = 0;
		detN = 0;

		double profnoise = sqrt(double(Nbins)/Tobs)*EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];


		double MLSigma = 0;
		int MLSigmaCount = 0;
		for(int j =0; j < Nbins; j++){
			double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - shapevec[j];
			//printf("%i %g %g \n", j, (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] , shapevec[j]);
			MLSigma += res*res; MLSigmaCount += 1;
		}
		MLSigma=sqrt(MLSigma/MLSigmaCount)*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];
		//printf("MLSigma: %g \n", MLSigma);
		for(int j =0; j < Nbins; j++){

			double noise = MLSigma*MLSigma;

			detN += log(noise);

			double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] ;//- OverallProfileAmp*shapevec[j];//-offset;

			if(shapevec[j] < 0)badshape = 1;
			NDiffVec[j] = datadiff/(noise);
			Chisq += datadiff*datadiff/(noise);
			
		 	for(int k =0; k < Msize; k++){
				NM[j][k] = M[j][k]/(noise);
		    	}


		
		}
		
		dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');
		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
			MNM[i][i] += pow(10.0, -12)*MNM[i][i];
		}


		info=0;
		Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);


	    
		logMargindet += Margindet;

		Marginlike = 0;
		for(int i =0; i < Msize; i++){
			Marginlike += TempdNM[i]*dNM[i];
		}
		//printf("%g %g %g %g \n", detN, Chisq, logMargindet, Marginlike);
		profilelike = -0.5*(detN + Chisq + logMargindet - Marginlike);
		lnew += profilelike;

		delete[] shapevec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] TempdNM;
		delete[] Betatimes;

		for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] M[j];
		    delete[] NM[j];
		}
		delete[] Hermitepoly;
		delete[] M;
		delete[] NM;

		for (int j = 0; j < Msize; j++){
		    delete[] MNM[j];
		}
		delete[] MNM;
	
	
	 }
	 
	 
	 
	 

	if(badshape == 1){
	//	lnew = -pow(10.0, 10.0);
	}
	if(isnan(lnew)){
		lnew= -pow(10.0, 10.0);
	}
	printf("Like: %.10g \n", lnew);
	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;


	return lnew;

}


double  AllTOAWriteMaxLike(std::string longname, int &ndim){



	double *Cube = new double[ndim];
        int number_of_lines = 0;

        std::ifstream checkfile;
        std::string checkname = longname+"_phys_live.txt";
        checkfile.open(checkname.c_str());
        std::string line;
        while (getline(checkfile, line))
                ++number_of_lines;

        checkfile.close();

	std::ifstream summaryfile;
	std::string fname = longname+"_phys_live.txt";
	summaryfile.open(fname.c_str());



	printf("Getting ML \n");
	double maxlike = -1.0*pow(10.0,10);
	for(int i=0;i<number_of_lines;i++){

		std::string line;
		getline(summaryfile,line);
		std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);

		double like = paramlist[ndim];

		if(like > maxlike){
			maxlike = like;
			 for(int i = 0; i < ndim; i++){
                        	 Cube[i] = paramlist[i];
                	 }
		}

	}
	summaryfile.close();

	double *EFAC;
	double *EQUAD;
        long double LDparams[ndim];
        int pcount;

	double uniformpriorterm = 0;


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){

		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

	}	
	
	
	pcount=0;
	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];
	//	printf("timing param %i %.25Lg	\n", p, LDparams[pcount]);
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
           	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
                	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){
                        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
			}
                 }
           }

	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form SATS///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}


        for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
                ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }

	delete[] JumpVec;

	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);      




/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////White noise/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			pcount++;
		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
                }
        }


        double *SQUAD;
	if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
        	SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
        	for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
           		SQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
        	}
    	}	  
	  





/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Red Noise///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	double start,end;
	int go=0;
	for (int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++)
	  {
	    if (((MNStruct *)globalcontext)->pulse->obsn[i].deleted==0)
	      {
		if (go==0)
		  {
		    go = 1;
		    start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    end  = start;
		  }
		else
		  {
		    if (start > (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    if (end < (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      end = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		  }
	      }
	  }

	double maxtspan=1*(end-start);


	
	int FitRedCoeff=2*(((MNStruct *)globalcontext)->numFitRedCoeff);
	int totCoeff=0;
	if(((MNStruct *)globalcontext)->incRED != 0)totCoeff+=FitRedCoeff;



	double FreqDet = 0;

	double *freqs = new double[totCoeff];
   	double *SignalCoeff = new double[totCoeff];

	for(int i = 0; i < totCoeff; i++){
		SignalCoeff[i] = 0;
	}	


	double *powercoeff=new double[totCoeff];
	for(int o=0;o<totCoeff; o++){
		powercoeff[o]=0;
	}
	
	double **FMatrix=new double*[((MNStruct *)globalcontext)->pulse->nobs];
	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		FMatrix[i]=new double[totCoeff];
		for(int j=0;j<totCoeff;j++){
			FMatrix[i][j]=0;
		}
	}


	int startpos=0;
	if(((MNStruct *)globalcontext)->incRED==5){

		FreqDet=0;

		for(int i = 0; i < FitRedCoeff; i++){
			SignalCoeff[startpos + i] = Cube[pcount];
			pcount++;
		}
			
		
		for(int pl = 0; pl < ((MNStruct *)globalcontext)->numFitRedPL; pl ++){

			double Tspan = maxtspan;
                        double f1yr = 1.0/3.16e7;


			double redamp=Cube[pcount];
			pcount++;
			double redindex=Cube[pcount];
			pcount++;

			
			redamp=pow(10.0, redamp);
			if(((MNStruct *)globalcontext)->RedPriorType ==1) { uniformpriorterm +=log(redamp); }



			double Agw=redamp;
			for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
				
				freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[i]/maxtspan;
				freqs[startpos+i+FitRedCoeff/2]=freqs[startpos+i];
				
				double rho=0;

 				rho = (Agw*Agw/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-redindex))/(maxtspan*24*60*60);

 				powercoeff[i]+= rho;
 				powercoeff[i+FitRedCoeff/2]+= rho;

				

			}
		}
		
		
		double GWBAmp=0;
		if(((MNStruct *)globalcontext)->incGWB==1){
			GWBAmp=pow(10.0,Cube[pcount]);
			pcount++;

			uniformpriorterm += log(GWBAmp);
			double Tspan = maxtspan;
			double f1yr = 1.0/3.16e7;
			for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
				double rho = (GWBAmp*GWBAmp/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-4.333))/(maxtspan*24*60*60);
				powercoeff[i]+= rho;
				powercoeff[i+FitRedCoeff/2]+= rho;
			}
		}

		for (int i=0; i<FitRedCoeff/2; i++){
			FreqDet=FreqDet+2*log(powercoeff[i]);
		}
		
		for(int i=0;i<FitRedCoeff/2;i++){
			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
				FMatrix[k][i]=cos(2*M_PI*freqs[i]*time);

			}
		}

		for(int i=0;i<FitRedCoeff/2;i++){
			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
				FMatrix[k][i+FitRedCoeff/2]=sin(2*M_PI*freqs[i]*time);
			}
		}

		startpos=FitRedCoeff;

    	}

	double *SignalVec = new double[((MNStruct *)globalcontext)->pulse->nobs];

	
	dgemv(FMatrix,SignalCoeff,SignalVec,((MNStruct *)globalcontext)->pulse->nobs,totCoeff,'N');

	double FreqLike = 0;
	for(int i = 0; i < totCoeff; i++){
		FreqLike += SignalCoeff[i]*SignalCoeff[i]/powercoeff[i];
	}



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	   
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;
	      }
	      
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr  - phase - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
	 }


	int maxshapecoeff = 0;
	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;
	int numshapestoccoeff = ((MNStruct *)globalcontext)->numshapestoccoeff;

	double shapecoeff[numcoeff];
	double StocProfCoeffs[numshapestoccoeff];

	for(int i =0; i < numcoeff; i++){
		if(((MNStruct *)globalcontext)->FixProfile==0){
			shapecoeff[i]= Cube[pcount];
			pcount++;
		}
		else{
			shapecoeff[i]=((MNStruct *)globalcontext)->MeanProfileShape[i];
			printf("Fixing mean: %i %g n", i, shapecoeff[i]);
		}
	}
	
	double beta=0;
	if(((MNStruct *)globalcontext)->FixProfile==0){
		beta = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;
		pcount++;
	}
	else{
		beta = ((MNStruct *)globalcontext)->MeanProfileBeta*((MNStruct *)globalcontext)->ReferencePeriod;
	}	

	for(int i =0; i < numshapestoccoeff; i++){
		StocProfCoeffs[i]= pow(10.0, 2*Cube[pcount]);
		//uniformpriorterm += log(pow(10.0, Cube[pcount]));
		pcount++;
	}

	if(numcoeff+1>=numshapestoccoeff+1){
		maxshapecoeff=numcoeff+1;
	}
	if(numshapestoccoeff+1 > numcoeff+1){
		maxshapecoeff=numshapestoccoeff+1;
	}

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	double lnew = 0;
	int badshape = 0;

	double *meanshape = new double[numcoeff];
	double *maxshape = new double[numcoeff];
	double *minshape = new double[numcoeff];

	 for(int c = 0; c < numcoeff; c++){
		meanshape[c] = 0;
		maxshape[c] = -1000;
		minshape[c] = 1000;
	}
			
        int minoffpulse=750;
        int maxoffpulse=1250;
       //int minoffpulse=500;
       //int maxoffpulse=1500;

	
	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){

		int nTOA = t;

		double detN  = 0;
		double Chisq  = 0;
		double logMargindet = 0;
		double Marginlike = 0;	 
		double JitterPrior = 0;	   

		double profilelike=0;

		long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
		long double FoldingPeriodDays = FoldingPeriod/SECDAY;
		int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
		double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
		double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
		long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

		double *Betatimes = new double[Nbins];
		double **AllHermiteBasis =  new double*[Nbins];
		double **JitterBasis  =  new double*[Nbins];
		double **Hermitepoly =  new double*[Nbins];

		for(int i =0;i<Nbins;i++){AllHermiteBasis[i]=new double[maxshapecoeff];for(int j =0;j<maxshapecoeff;j++){AllHermiteBasis[i][j]=0;}}
		for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}
		for(int i =0;i<Nbins;i++){JitterBasis[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){JitterBasis[i][j]=0;}}
	
	



	

		double sigma=noiseval;
		sigma=pow(10.0, sigma);
		//printf("noise details %i %g %g %g \n", t, noiseval, Tobs, EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]]);

		long double binpos = ModelBats[nTOA];

		if(((MNStruct *)globalcontext)->incRED==5){
			long double rednoiseshift = (long double) SignalVec[t];
			binpos+=rednoiseshift/SECDAY;
			//printf("Red noise shift %i %Lg \n", t, rednoiseshift);
		}

		if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

		long double minpos = binpos - FoldingPeriodDays/2;
		if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];


	    
		for(int j =0; j < Nbins; j++){
			long double timediff = 0;
			long double bintime = ProfileBats[t][j];
			if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
			}
			else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
			}
			else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
			}

			timediff=timediff*SECDAY;


			Betatimes[j]=timediff/beta;
			TNothpl(maxshapecoeff,Betatimes[j],AllHermiteBasis[j]);

			for(int k =0; k < maxshapecoeff; k++){

				double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));//*((MNStruct *)globalcontext)->Factorials[k]);
				AllHermiteBasis[j][k]=AllHermiteBasis[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);
			//	if(nTOA==0 && j == 0){printf("%i %g \n", k, AllHermiteBasis[j][k]);}
				if(k<numcoeff){ Hermitepoly[j][k] = AllHermiteBasis[j][k];}
			
			}

			JitterBasis[j][0] = (1.0/sqrt(2.0))*(-1.0*AllHermiteBasis[j][1]);
			for(int k =1; k < numcoeff; k++){
				JitterBasis[j][k] = (1.0/sqrt(2.0))*(sqrt(double(k))*AllHermiteBasis[j][k-1] - sqrt(double(k+1))*AllHermiteBasis[j][k+1]);
			}	
	    }


	    double *shapevec  = new double[Nbins];
	    double *Jittervec = new double[Nbins];
	
	    double OverallProfileAmp = shapecoeff[0];
	    
	    shapecoeff[0] = 1;

	    dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');
    	    dgemv(JitterBasis,shapecoeff,Jittervec,Nbins,numcoeff,'N');
	
	    double *NDiffVec = new double[Nbins];

	    double maxshape=0;
	    double modelflux=0;
	    double testmodelflux=0;	
            double dataflux = 0;


	    for(int j =0; j < Nbins; j++){

		    detN += log(sigma*sigma);

		    double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] ;//- OverallProfileAmp*shapevec[j];//-offset;

		    if(shapevec[j] < 0)badshape = 1;
		    NDiffVec[j] = datadiff/(sigma*sigma);

		    if(shapevec[j] > maxshape){ maxshape = shapevec[j];}


		    testmodelflux += shapevec[j]*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;

	    }
	    for(int j =0; j < numcoeff; j=j+2){
		modelflux+=sqrt(sqrt(M_PI))*sqrt(beta)*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Binomial[j])*shapecoeff[j];
		//printf("binomial %i %g %g\n", j, ((MNStruct *)globalcontext)->Binomial[j], ((MNStruct *)globalcontext)->Factorials[j]/(((MNStruct *)globalcontext)->Factorials[j - j/2]*((MNStruct *)globalcontext)->Factorials[j/2]));
	   }

	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


		double **M = new double*[Nbins];
		double **NM = new double*[Nbins];

		int Msize = 2+numshapestoccoeff;
		int Mcounter=0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
			Msize++;
		}


		for(int i =0; i < Nbins; i++){
			Mcounter=0;
			M[i] = new double[Msize];
			NM[i] = new double[Msize];

			M[i][0] = 1;
			M[i][1] = shapevec[i];

			Mcounter = 2;

			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
//				printf("%i %i %g %g %g \n", nTOA, i, M[i][0], M[i][1], M[i][2]);
				M[i][Mcounter] = Jittervec[i];
				Mcounter++;
			}			



			for(int j = 0; j < numshapestoccoeff; j++){

			    M[i][Mcounter+j] = AllHermiteBasis[i][j];

			}

			for(int j = 0; j < Msize; j++){
				NM[i][j] = M[i][j]/(sigma*sigma);
		//		printf("M %i %i %g %g \n",i,j,M[i][j], sigma); 
			}
		  
		}


		double **MNM = new double*[Msize];
		for(int i =0; i < Msize; i++){
		    MNM[i] = new double[Msize];
		}

		dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');
//		printf("MNM %g %g %g %g \n", MNM[0][0], MNM[0][1], MNM[1][0], MNM[1][1]);

		for(int j = 2; j < Msize; j++){
			MNM[j][j] += pow(10.0,20);
		}

		double *dNM = new double[Msize];
		double *TempdNM = new double[Msize];
		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
		}


		int info=0;
		double Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);


		double maxoffset = TempdNM[0];
		double maxamp = TempdNM[1];



		printf("max %i %g %g \n", t, maxoffset, maxamp);

		      
		Chisq = 0;
		detN = 0;

		double noisemean=0.0;

		for(int j =minoffpulse; j < maxoffpulse; j++){
			noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
		}

		noisemean=noisemean/(maxoffpulse-minoffpulse);
		double MLSigma = 0;
		double MLSigma2 = 0;
		int MLSigmaCount2 = 0;
		int MLSigmaCount = 0;
		for(int j =0; j < Nbins; j++){
		      double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - maxamp*shapevec[j] - maxoffset;
		      //if(shapevec[j] <= maxshape*0.001){MLSigma += res*res; MLSigmaCount += 1; printf("Adding to Noise %i %i %g %g \n", t, j,(double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1], maxamp*shapevec[j] + maxoffset);}
			if(j > minoffpulse && j < maxoffpulse){
			//	if(nTOA == 0){printf("res: %i %g \n", j, res);}
				MLSigma += ((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean)*((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean); MLSigmaCount += 1;
			}
			else{
				MLSigma2 += res*res; MLSigmaCount2 += 1;
			}
		}

		MLSigma = sqrt(MLSigma/MLSigmaCount);
		MLSigma2 = sqrt(MLSigma2/MLSigmaCount2);
		double tempdataflux=0;
		for(int j =0; j < Nbins; j++){
			if(shapevec[j] > maxshape*0.001){
				if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - maxoffset)/MLSigma > 1){
					dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-maxoffset)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
					tempdataflux += ((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-maxoffset);
			//		if(t==1){
                          //                      printf("dataflux %i %g %g %g %g %g\n", j, maxoffset, ((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]), dataflux, tempdataflux, double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24);
                            //            }

				}
			}
		}	

 
//		printf("TOA %i %g %g %g %g\n", t, EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]], MLSigma, MLSigma2, MLSigma2/MLSigma);
		MLSigma = MLSigma;//*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];

//		double highfreqnoise = sqrt(double(Nbins)/Tobs)*SQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
		double highfreqnoise = SQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
		double flatnoise = EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];

//		double *testflux=new double[numshapestoccoeff];
//		for(int j = 0; j < numshapestoccoeff; j++){
//			testflux[j] = 0;
//		}



		double *profilenoise = new double[Nbins];
		for(int i =0; i < Nbins; i++){
		  	Mcounter=2;
			double noise = MLSigma*MLSigma +  pow(highfreqnoise*maxamp*shapevec[i],2);

			if(shapevec[i] > maxshape*0.001){
				noise +=  pow(flatnoise*maxamp*maxshape,2);
			}


			profilenoise[i] = sqrt(noise);
			//printf("detN: %i %i %g %g %g %g\n", nTOA, i, MLSigma, highfreqnoise, maxamp, noise);
			detN += log(noise);

			double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1];

			if(shapevec[i] < 0)badshape = 1;
			NDiffVec[i] = datadiff/(noise);
		       

			Chisq += datadiff*datadiff/(noise);
	
			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
				M[i][2] = M[i][2]*dataflux/modelflux/beta;
				Mcounter++;
			}
			
		   	for(int j = 0; j < numshapestoccoeff; j++){
//				testflux[j] += pow(M[i][Mcounter+j]*sqrt(double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24)*dataflux, 2);
//				M[i][Mcounter+j] = M[i][Mcounter+j]*dataflux*sqrt(Tobs/double(Nbins));
				M[i][Mcounter+j] = M[i][Mcounter+j]*dataflux;

		    	}


			for(int j = 0; j < Msize; j++){
				NM[i][j] = M[i][j]/(noise);
			}


		}
		
//		for(int j = 0; j < numshapestoccoeff; j++){
//			printf("test flux %i %i %g %g \n", t, j, testflux[j], pow(dataflux,2));
//		}
//		delete[] testflux;
			
		dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');


		Mcounter=2;
		double JitterDet = 0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
			double profnoise = EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];//sqrt(double(Nbins)/Tobs)*EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
			profnoise = profnoise*profnoise;
		    	JitterDet = log(profnoise);
		    	MNM[2][2] += 1.0/profnoise;
			Mcounter++;
		}


		double StocProfDet = 0;
		for(int j = 0; j < numshapestoccoeff; j++){
			StocProfDet += log(StocProfCoeffs[j]);
			MNM[Mcounter+j][Mcounter+j] += 1.0/StocProfCoeffs[j];
		}

		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
		}


		info=0;
		Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);

		Marginlike = 0;
                for(int i =0; i < Msize; i++){
                        Marginlike += TempdNM[i]*dNM[i];
                }



		double finaloffset = TempdNM[0];
		double finalamp = TempdNM[1];
		double finalJitter = TempdNM[2];

		double *StocVec = new double[Nbins];


		dgemv(M,TempdNM,shapevec,Nbins,Msize,'N');


		 for(int c = 0; c < numcoeff; c++){
			meanshape[c] += shapecoeff[c];
		}
		

		TempdNM[0] = 0;
		TempdNM[1] = 0;
		TempdNM[2] = 0;

                 for(int c = 3; c < Msize; c++){
                        meanshape[c-3] += (TempdNM[c]/finalamp)*dataflux;
			//printf("stoc: %i %g %g\n", c, (TempdNM[c]/finalamp)*dataflux, (TempdNM[c]/finalamp)/dataflux);
                }


		dgemv(M,TempdNM,StocVec,Nbins,Msize,'N');

		std::ofstream profilefile;
		std::string ProfileName =  ((MNStruct *)globalcontext)->pulse->obsn[t].fname;
		std::string dname = longname+ProfileName+"-Profile.txt";
	
		profilefile.open(dname.c_str());
		double MLSigma3 = 0;	
		double MLWeight3 = 0;	
		for(int i =0; i < Nbins; i++){
			MLSigma3 += pow(((double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1]-shapevec[i]),2)*pow(1.0/profilenoise[i], 2);
			MLWeight3 += pow(1.0/profilenoise[i], 2);
			profilefile << i << " " << std::setprecision(10) << (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1] << " " << shapevec[i] << " " << profilenoise[i] << " " << finaloffset + finalamp*M[i][1] << " " << finalJitter*M[i][2] << " " << finaloffset + StocVec[i] << "\n";

		}
		printf("MLsigma: %s %g %g %g \n", ProfileName.c_str(), MLSigma, MLSigma2, MLSigma2/MLSigma);
	    	profilefile.close();
		delete[] profilenoise;
		delete[] StocVec;

		logMargindet = Margindet;


		profilelike = -0.5*(detN + Chisq + logMargindet + StocProfDet + JitterDet - Marginlike);
		lnew += profilelike;
//		printf("Like: %i  %.15g %.15g %.15g %.15g %.15g %.15g\n", nTOA,detN, Chisq, logMargindet, Marginlike, StocProfDet , JitterDet);

		delete[] shapevec;
		delete[] Jittervec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] Betatimes;

		for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] JitterBasis[j];
		    delete[] AllHermiteBasis[j];
		    delete[] M[j];
		    delete[] NM[j];
		}
		delete[] Hermitepoly;
		delete[] AllHermiteBasis;
		delete[] JitterBasis;
		delete[] M;
		delete[] NM;

		for (int j = 0; j < Msize; j++){
		    delete[] MNM[j];
		}
		delete[] MNM;
	
	
	}
	 
	 
	 
	 for(int c = 0; c < numcoeff; c++){
		double mval = meanshape[c]/meanshape[0];
		printf("ShapePriors[%i][0] = %.10g         %.10g\n", c, mval-mval*0.5, mval);
		printf("ShapePriors[%i][1] = %.10g         %.10g\n",c, mval+mval*0.5, mval);
	}
	 

	if(badshape == 1){
	//	lnew = -pow(10.0, 10.0);
	}

	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;
	delete[] powercoeff;
	delete[] freqs;
	delete[] SignalCoeff;
	delete[] SignalVec;
	
	for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
		delete[]FMatrix[j];
	}
	delete[]FMatrix;


	printf("lnew: %g \n", lnew);

	return lnew;

}


double  AllTOAMarginStocProfLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){

	printf("in like\n");

	double *EFAC;
	double *EQUAD;
        long double LDparams[ndim];
        int pcount;

	double uniformpriorterm = 0;


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){

		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

	}	
	
	
	pcount=0;
	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];
	//	printf("timing param %i %.25Lg	\n", p, LDparams[pcount]);
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
           	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
                	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){
                        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
			}
                 }
           }

	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form SATS///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}


        for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
                ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }

	delete[] JumpVec;

	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);      
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);       




/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////White noise/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[0]);}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[p]);}
			pcount++;
		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
                }
        }


        double *SQUAD;
	if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
        	SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
        	for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
           		SQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
        	}
    	}	  
	  





/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Red Noise///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	double start,end;
	int go=0;
	for (int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++)
	  {
	    if (((MNStruct *)globalcontext)->pulse->obsn[i].deleted==0)
	      {
		if (go==0)
		  {
		    go = 1;
		    start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    end  = start;
		  }
		else
		  {
		    if (start > (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    if (end < (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      end = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		  }
	      }
	  }

	double maxtspan=1*(end-start);


	
	int FitRedCoeff=2*(((MNStruct *)globalcontext)->numFitRedCoeff);
	int totCoeff=0;
	if(((MNStruct *)globalcontext)->incRED != 0)totCoeff+=FitRedCoeff;



	double FreqDet = 0;

	double *freqs = new double[totCoeff];
   	double *SignalCoeff = new double[totCoeff];

	for(int i = 0; i < totCoeff; i++){
		SignalCoeff[i] = 0;
	}	


	double *powercoeff=new double[totCoeff];
	for(int o=0;o<totCoeff; o++){
		powercoeff[o]=0;
	}
	
	double **FMatrix=new double*[((MNStruct *)globalcontext)->pulse->nobs];
	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		FMatrix[i]=new double[totCoeff];
		for(int j=0;j<totCoeff;j++){
			FMatrix[i][j]=0;
		}
	}


	int startpos=0;
	if(((MNStruct *)globalcontext)->incRED==5){

		FreqDet=0;

		for(int i = 0; i < FitRedCoeff; i++){
			SignalCoeff[startpos + i] = Cube[pcount];
			pcount++;
		}
			
		
		for(int pl = 0; pl < ((MNStruct *)globalcontext)->numFitRedPL; pl ++){

			double Tspan = maxtspan;
                        double f1yr = 1.0/3.16e7;


			double redamp=Cube[pcount];
			pcount++;
			double redindex=Cube[pcount];
			pcount++;

			
			redamp=pow(10.0, redamp);
			if(((MNStruct *)globalcontext)->RedPriorType ==1) { uniformpriorterm +=log(redamp); }



			double Agw=redamp;
			for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
				
				freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[i]/maxtspan;
				freqs[startpos+i+FitRedCoeff/2]=freqs[startpos+i];
				
				double rho=0;

 				rho = (Agw*Agw/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-redindex))/(maxtspan*24*60*60);

 				powercoeff[i]+= rho;
 				powercoeff[i+FitRedCoeff/2]+= rho;

				

			}
		}
		
		
		double GWBAmp=0;
		if(((MNStruct *)globalcontext)->incGWB==1){
			GWBAmp=pow(10.0,Cube[pcount]);
			pcount++;

			uniformpriorterm += log(GWBAmp);
			double Tspan = maxtspan;
			double f1yr = 1.0/3.16e7;
			for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
				double rho = (GWBAmp*GWBAmp/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-4.333))/(maxtspan*24*60*60);
				powercoeff[i]+= rho;
				powercoeff[i+FitRedCoeff/2]+= rho;
			}
		}

		for (int i=0; i<FitRedCoeff/2; i++){
			FreqDet=FreqDet+2*log(powercoeff[i]);
		}
		
		for(int i=0;i<FitRedCoeff/2;i++){
			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
				FMatrix[k][i]=cos(2*M_PI*freqs[i]*time);

			}
		}

		for(int i=0;i<FitRedCoeff/2;i++){
			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
				FMatrix[k][i+FitRedCoeff/2]=sin(2*M_PI*freqs[i]*time);
			}
		}

		startpos=FitRedCoeff;

    	}

	double *SignalVec = new double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
		SignalVec[k] = 0;
	}

	
	dgemv(FMatrix,SignalCoeff,SignalVec,((MNStruct *)globalcontext)->pulse->nobs,totCoeff,'N');

	double FreqLike = 0;
	for(int i = 0; i < totCoeff; i++){
		FreqLike += SignalCoeff[i]*SignalCoeff[i]/powercoeff[i];
	}



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract  DM Shape Events////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	if(((MNStruct *)globalcontext)->incDMShapeEvent != 0){
                for(int i =0; i < ((MNStruct *)globalcontext)->incDMShapeEvent; i++){

			int numDMShapeCoeff=((MNStruct *)globalcontext)->numDMShapeCoeff;

                        double EventPos=Cube[pcount];
			pcount++;
                        double EventWidth=Cube[pcount];
			pcount++;



			double *DMshapecoeff=new double[numDMShapeCoeff];
			double *DMshapeVec=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapecoeff[c]=Cube[pcount];
				pcount++;
			}


			double *DMshapeNorm=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapeNorm[c]=1.0/sqrt(sqrt(2.0*M_PI)*pow(2.0,c)*iter_factorial(c));
			}

			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

				double HVal=(time-EventPos)/(sqrt(2.0)*EventWidth);
				TNothpl(numDMShapeCoeff,HVal,DMshapeVec);
				double DMsignal=0;
				for(int c=0; c < numDMShapeCoeff; c++){
					DMsignal += DMshapeNorm[c]*DMshapeVec[c]*DMshapecoeff[c];
				}

				  SignalVec[k] += DMsignal*exp(-0.5*pow((time-EventPos)/EventWidth, 2));
	
			}




		delete[] DMshapecoeff;
		delete[] DMshapeVec;
		delete[] DMshapeNorm;

		}
	}



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	   
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;
	      }
	      
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr  - phase - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
	 }


	int maxshapecoeff = 0;
	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;
	int numshapestoccoeff = ((MNStruct *)globalcontext)->numshapestoccoeff;


	double StocProfCoeffs[numshapestoccoeff];
	double profileamps[((MNStruct *)globalcontext)->pulse->nobs];

	profileamps[0] = 1;
	for(int i =1; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
		profileamps[i]= Cube[pcount];
		pcount++;
	}

	double beta = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;

	pcount++;

	for(int i =0; i < numshapestoccoeff; i++){
		StocProfCoeffs[i]= pow(10.0, 2*Cube[pcount]);
		//uniformpriorterm += log(pow(10.0, Cube[pcount]));
		pcount++;
	}

	if(numcoeff+1>numshapestoccoeff+1){
		maxshapecoeff=numcoeff+1;
	}
	if(numshapestoccoeff+1 > numcoeff+1){
		maxshapecoeff=numshapestoccoeff+1;
	}

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	printf("starting profile stuff \n");
	double lnew = 0;
	int badshape = 0;

	int totalcoeff = numcoeff;
	int totalbins = 0;
	
	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
		totalbins  += (int)((MNStruct *)globalcontext)->ProfileInfo[t][1];
		totalcoeff += numshapestoccoeff;
		totalcoeff += 1;    //For the arbitrary offset
	}

	double *AllNoise = new double[totalbins];
	double *AllD = new double[totalbins];
	double *dNM = new double[totalcoeff];
	double *TempdNM = new double[totalcoeff];

	double **AllProfiles = new double*[totalcoeff];
	double **NAllProfiles = new double*[totalcoeff];
	for(int t = 0; t < totalcoeff; t++){
		AllProfiles[t] = new double[totalbins];
		NAllProfiles[t] = new double[totalbins];
		for(int i = 0; i < totalcoeff; i++){
			AllProfiles[t][i] = 0;
			NAllProfiles[t][i] = 0;
		}
	}
	
	int stocprofcount = numcoeff+((MNStruct *)globalcontext)->pulse->nobs;
	int bincount = 0;
	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){



		double MLSigma = 0;
		int MLSigmaCount = 0;
		double noisemean=0;

		for(int j =500; j < 1500; j++){
			noisemean += (double)((MNStruct *)globalcontext)->ProfileData[t][j][1];
			MLSigmaCount += 1;
		}

		noisemean = noisemean/MLSigmaCount;

		for(int j =500; j < 1500; j++){
			double res = (double)((MNStruct *)globalcontext)->ProfileData[t][j][1] - noisemean;
			MLSigma += res*res; 
		}

		MLSigma = sqrt(MLSigma/MLSigmaCount);


		printf("toa %i sig %g \n", t, MLSigma);

		int nTOA = t;


		long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
		long double FoldingPeriodDays = FoldingPeriod/SECDAY;
		int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
		double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
		double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
		long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

		double *Betatimes = new double[Nbins];
		double **AllHermiteBasis =  new double*[Nbins];
		double **Hermitepoly =  new double*[Nbins];

		for(int i =0;i<Nbins;i++){AllHermiteBasis[i]=new double[maxshapecoeff];for(int j =0;j<maxshapecoeff;j++){AllHermiteBasis[i][j]=0;}}
		for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}

	

		long double binpos = ModelBats[nTOA];

	//	if(((MNStruct *)globalcontext)->incRED==5){
			long double rednoiseshift = (long double) SignalVec[t];
			binpos+=rednoiseshift/SECDAY;
			//printf("Red noise shift %i %Lg \n", t, rednoiseshift);
	//	}

		if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

		long double minpos = binpos - FoldingPeriodDays/2;
		if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];


	    	double flatnoise = EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]]*profileamps[t];


		for(int j =0; j < Nbins; j++){

			AllD[bincount+j] =  (double)((MNStruct *)globalcontext)->ProfileData[t][j][1];

			long double timediff = 0;
			long double bintime = ProfileBats[t][j];
			if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
			}
			else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
			}
			else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
			}

			timediff=timediff*SECDAY;


			Betatimes[j]=timediff/beta;
			TNothpl(maxshapecoeff,Betatimes[j],AllHermiteBasis[j]);

			//printf("%i %i %g \n", t, j, AllD[bincount+j]);
			for(int k =0; k < maxshapecoeff; k++){
				//printf("%i %i %i %i %i\n", k, stocprofcount+k, bincount+j, totalcoeff, totalbins);
				double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));
				AllHermiteBasis[j][k]=AllHermiteBasis[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);

				if(k < numcoeff){ 
					AllProfiles[k][bincount+j] = AllHermiteBasis[j][k]*profileamps[t];
				}

				if(k < numshapestoccoeff){ 
					AllProfiles[stocprofcount+k][bincount+j] = AllHermiteBasis[j][k]*profileamps[t];
				}
			
			}

			AllProfiles[numcoeff+t][bincount+j] = 1;


			AllNoise[bincount+j] = MLSigma*MLSigma;
			if(j < 500 || j > 1500){ 
				AllNoise[bincount+j] += flatnoise*flatnoise;
			}



	   	 }

		stocprofcount += numshapestoccoeff;
		bincount += Nbins;

		delete[] Betatimes;

		for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] AllHermiteBasis[j];
		}
		delete[] Hermitepoly;
		delete[] AllHermiteBasis;

	
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Do the Linear Algebra///////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	int SVDSize = numcoeff+((MNStruct *)globalcontext)->pulse->nobs;
	double **M = new double*[totalbins];
	for(int j =0; j < totalbins; j++){
	 	M[j] = new double[SVDSize];
		for(int k =0; k < SVDSize; k++){
			M[j][k] = AllProfiles[k][j];
		}
	}
	 

	double* S = new double[SVDSize];
	double** U = new double*[totalbins];
	for(int k=0; k < totalbins; k++){
		U[k] = new double[totalbins];
	}
	double** VT = new double*[SVDSize]; 
	for (int k=0; k<SVDSize; k++) VT[k] = new double[SVDSize];

	printf("doing the svd \n"); 
	dgesvd(M,totalbins, SVDSize, S, U, VT);
	printf("done\n");

	for(int j =0; j < totalbins; j++){
		for(int k =0; k < SVDSize; k++){
			AllProfiles[k][j] = U[j][k];
		}
	}

	delete[]S;	

	for (int j = 0; j < SVDSize; j++){
		delete[]VT[j];
	}

	delete[]VT;	


 
	for (int j = 0; j < totalbins; j++){
		delete[] U[j];
		delete[] M[j];
	}

	delete[]U;
	delete[]M;


	for(int j =0; j < totalbins; j++){
		for(int k =0; k < SVDSize; k++){
			AllProfiles[k][j] = U[j][k];
		}
	}


	double detN = 0;
	for(int j =0; j < totalbins; j++){
		detN += log(AllNoise[j]);
		for(int k =0; k < totalcoeff; k++){
			NAllProfiles[k][j] = AllProfiles[k][j]/AllNoise[j];
		}
	}

	 dgemv(NAllProfiles,AllD,dNM,totalcoeff,totalbins,'N');

	double **MNM = new double*[totalcoeff];
	for(int i =0; i < totalcoeff; i++){
		MNM[i] = new double[totalcoeff];
	}

	printf("doing dgemm \n");
	dgemm(NAllProfiles, AllProfiles , MNM, totalcoeff, totalbins,totalcoeff, totalbins, 'N', 'T');
 	printf("done\n");

	stocprofcount = numcoeff+((MNStruct *)globalcontext)->pulse->nobs;
	double StocProfDet = 0;
	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
		for(int j = 0; j < numshapestoccoeff; j++){
			StocProfDet += log(StocProfCoeffs[j]);
			MNM[stocprofcount+j][stocprofcount+j] += 1.0/StocProfCoeffs[j];
		}

		stocprofcount += numshapestoccoeff;
	}

	int info=0;
	double Margindet = 0;
	dpotrfInfo(MNM, totalcoeff, Margindet, info);
	dpotrs(MNM, TempdNM, totalcoeff);

	double Marginlike = 0;
        for(int i =0; i < totalcoeff; i++){
                Marginlike += TempdNM[i]*dNM[i];
        }

	lnew = -0.5*(detN + StocProfDet + Margindet - Marginlike);

	if(badshape == 1){
	//	lnew = -pow(10.0, 10.0);
	}

	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;
	delete[] powercoeff;
	delete[] freqs;
	delete[] SignalCoeff;
	delete[] SignalVec;
	
	for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
		delete[]FMatrix[j];
	}
	delete[]FMatrix;


	lnew += -0.5*(FreqDet + FreqLike) + uniformpriorterm;
	printf("Like: %.10g %g %g %g %g\n", lnew, detN , StocProfDet , Margindet , Marginlike);

	return lnew;

}


*/
void TemplateProfLikeMNWrap(double *Cube, int &ndim, int &npars, double &lnew, void *context){



        for(int p=0;p<ndim;p++){

                Cube[p]=(((MNStruct *)globalcontext)->PriorsArray[p+ndim]-((MNStruct *)globalcontext)->PriorsArray[p])*Cube[p]+((MNStruct *)globalcontext)->PriorsArray[p];
        }


	double *DerivedParams = new double[npars];

	double result = TemplateProfLike(ndim, Cube, npars, DerivedParams, context);


	delete[] DerivedParams;

	lnew = result;

}

double  TemplateProfLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){

	//printf("In like \n");

	double *EFAC;
	double *EQUAD;
	double TemplateFlux=0;
	double FakeRMS = 0;
	double TargetSN=0;
        long double LDparams;
        int pcount;

	double uniformpriorterm = 0;


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	LDparams=Cube[0]*(((MNStruct *)globalcontext)->LDpriors[0][1]) + (((MNStruct *)globalcontext)->LDpriors[0][0]);
	
	
	pcount=0;
	long double phase = LDparams*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	TemplateFlux=0;
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	   
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0];
	      }
	      
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileData[i][0][0]+phase;
	 }

	int totalProfCoeff = 0;
	int *numcoeff= new int[((MNStruct *)globalcontext)->numProfComponents];
	//numcoeff[0] = ((MNStruct *)globalcontext)->numshapecoeff;
	//numcoeff[1] = 10;
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		numcoeff[i] =  ((MNStruct *)globalcontext)->numshapecoeff[i];
		totalProfCoeff += numcoeff[i];
	}

	double beta = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;

	pcount++;

	long double *CompSeps = new long double[((MNStruct *)globalcontext)->numProfComponents];
	CompSeps[0] = 0;
	for(int i = 1; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		CompSeps[i] = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;
		pcount++;
		//printf("CompSep: %g \n", Cube[pcount-1]);
	}
	

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	double lnew = 0;
	int badshape = 0;

	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
	//	printf("In toa %i \n", t);
		int nTOA = t;

		double detN  = 0;
		double logMargindet = 0;
		double Marginlike = 0;	 

		double profilelike=0;

		long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
		long double FoldingPeriodDays = FoldingPeriod/SECDAY;
		int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
		double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
		long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

		double *Betatimes = new double[Nbins];
		double **Hermitepoly =  new double*[Nbins];

		for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[totalProfCoeff];for(int j =0;j<totalProfCoeff;j++){Hermitepoly[i][j]=0;}}
	

		long double binpos = ModelBats[nTOA];


		if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

		long double minpos = binpos - FoldingPeriodDays/2;
		if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];


	    	int cpos = 0;
		for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
			for(int j =0; j < Nbins; j++){
				long double timediff = 0;
				long double bintime = ProfileBats[t][j]+CompSeps[i]/SECDAY;
				if(bintime  >= minpos && bintime <= maxpos){
				    timediff = bintime - binpos;
				}
				else if(bintime < minpos){
				    timediff = FoldingPeriodDays+bintime - binpos;
				}
				else if(bintime > maxpos){
				    timediff = bintime - FoldingPeriodDays - binpos;
				}

				timediff=timediff*SECDAY;
			

				Betatimes[j]=(timediff)/beta;
				TNothplMC(numcoeff[i],Betatimes[j],Hermitepoly[j], cpos);

				for(int k =0; k < numcoeff[i]; k++){
					//if(k==0)printf("HP %i %i %g %g \n", i, j, Betatimes[j],Hermitepoly[j][cpos+k]*exp(-0.5*Betatimes[j]*Betatimes[j]));
					double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));//*((MNStruct *)globalcontext)->Factorials[k]);
					Hermitepoly[j][cpos+k]=Hermitepoly[j][cpos+k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);

				}
					
			}
			cpos += numcoeff[i];
	   	 }

	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


		double **M = new double*[Nbins];

		int Msize = totalProfCoeff+1;
		for(int i =0; i < Nbins; i++){
			M[i] = new double[Msize];

			M[i][0] = 1;


			for(int j = 0; j < totalProfCoeff; j++){
				M[i][j+1] = Hermitepoly[i][j];
				//if(j==0)printf("%i %i %g \n", i, j, M[i][j+1]);
			}
		  
		}


		double **MNM = new double*[Msize];
		for(int i =0; i < Msize; i++){
		    MNM[i] = new double[Msize];
		}

		dgemm(M, M , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');

		for(int j = 0; j < Msize; j++){
			MNM[j][j] += pow(10.0,-14);
		}
	
		double minFlux = pow(10.0,100); 
		for(int j = 0; j < Nbins; j++){
			if((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] < minFlux){minFlux = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];}
		}

		double *dNM = new double[Msize];
		double *TempdNM = new double[Msize];

		double *NDiffVec = new double[Nbins];
		TemplateFlux = 0;
		for(int j = 0; j < Nbins; j++){
			TemplateFlux +=  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - minFlux;
			NDiffVec[j] = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - minFlux;
		}
		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
		}


		int info=0;
		double Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);


//		printf("Margindet %g \n", Margindet);
		double *shapevec = new double[Nbins];
		dgemv(M,TempdNM,shapevec,Nbins,Msize,'N');


		TargetSN = 10000.0;
		FakeRMS = TemplateFlux/TargetSN;
		FakeRMS = 1.0/(FakeRMS*FakeRMS);
		    double Chisq=0;
		    for(int j =0; j < Nbins; j++){


			    double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - shapevec[j] - minFlux;
			    Chisq += datadiff*datadiff*FakeRMS;
//				printf("%i %.10g %.10g \n", j, (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] , shapevec[j]);		

		    }

		profilelike = -0.5*(Chisq);
		lnew += profilelike;

		delete[] shapevec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] Betatimes;

		for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] M[j];
		}
		delete[] Hermitepoly;
		delete[] M;

		for (int j = 0; j < Msize; j++){
		    delete[] MNM[j];
		}
		delete[] MNM;
	
	
	}
	 
	 
	 
	 

	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] numcoeff;
	delete[] CompSeps;

	//printf("End Like: %.10g %g %g\n", lnew, TemplateFlux, FakeRMS);

	return lnew;

}

void  WriteMaxTemplateProf(std::string longname, int &ndim){

	//printf("In like \n");


        double *Cube = new double[ndim];
        int number_of_lines = 0;

        std::ifstream checkfile;
	std::string checkname;
	if(((MNStruct *)globalcontext)->sampler == 0){
	        checkname = longname+"phys_live.points";
	}
        if(((MNStruct *)globalcontext)->sampler == 1){
                checkname = longname+"_phys_live.txt";
        }
	printf("%i %s \n", ((MNStruct *)globalcontext)->sampler, checkname.c_str());
        checkfile.open(checkname.c_str());
        std::string line;
        while (getline(checkfile, line))
                ++number_of_lines;

        checkfile.close();

        std::ifstream summaryfile;
	std::string fname;
	if(((MNStruct *)globalcontext)->sampler == 0){
		fname = longname+"phys_live.points";
	}
	if(((MNStruct *)globalcontext)->sampler == 1){
	        fname = longname+"_phys_live.txt";
	}
        summaryfile.open(fname.c_str());



        printf("Getting ML \n");
        double maxlike = -1.0*pow(10.0,10);
        for(int i=0;i<number_of_lines;i++){

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);

                double like = paramlist[ndim];

                if(like > maxlike){
                        maxlike = like;
                         for(int i = 0; i < ndim; i++){
                                 Cube[i] = paramlist[i];
                         }
                }

        }
        summaryfile.close();


	double *EFAC;
	double *EQUAD;
	double TemplateFlux=0;
	double FakeRMS = 0;
	double TargetSN=0;
        long double LDparams;
        int pcount;

	double uniformpriorterm = 0;


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	LDparams=Cube[0]*(((MNStruct *)globalcontext)->LDpriors[0][1]) + (((MNStruct *)globalcontext)->LDpriors[0][0]);
	
	
	pcount=0;
	long double phase = LDparams*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	TemplateFlux=0;
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	   
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0];
	      }
	      
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileData[i][0][0]+phase;
	 }

	int totalProfCoeff = 0;
	int *numcoeff= new int[((MNStruct *)globalcontext)->numProfComponents];

	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		numcoeff[i] =  ((MNStruct *)globalcontext)->numshapecoeff[i];
		totalProfCoeff += numcoeff[i];
	}

	double beta = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;

	pcount++;

	long double *CompSeps = new long double[((MNStruct *)globalcontext)->numProfComponents];
	CompSeps[0] = 0;
	for(int i = 1; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		CompSeps[i] = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;
		pcount++;
		//printf("CompSep: %g \n", Cube[pcount-1]);
	}
/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	double lnew = 0;
	int badshape = 0;

	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
	//	printf("In toa %i \n", t);
		int nTOA = t;

		double detN  = 0;
		double logMargindet = 0;
		double Marginlike = 0;	 

		double profilelike=0;

		long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
		long double FoldingPeriodDays = FoldingPeriod/SECDAY;
		int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
		double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
		long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

		double *Betatimes = new double[Nbins];
		double **Hermitepoly =  new double*[Nbins];

		for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[totalProfCoeff];for(int j =0;j<totalProfCoeff;j++){Hermitepoly[i][j]=0;}}
	

		long double binpos = ModelBats[nTOA];


		if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

		long double minpos = binpos - FoldingPeriodDays/2;
		if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];


		int cpos = 0;
		for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
			printf("Sep %.10Lg \n", CompSeps[i]);
			for(int j =0; j < Nbins; j++){
				long double timediff = 0;
				long double bintime = ProfileBats[t][j]+CompSeps[i]/SECDAY;
				if(bintime  >= minpos && bintime <= maxpos){
				    timediff = bintime - binpos;
				}
				else if(bintime < minpos){
				    timediff = FoldingPeriodDays+bintime - binpos;
				}
				else if(bintime > maxpos){
				    timediff = bintime - FoldingPeriodDays - binpos;
				}

				timediff=timediff*SECDAY;
			

				Betatimes[j]=(timediff)/beta;
				TNothplMC(numcoeff[i],Betatimes[j],Hermitepoly[j], cpos);

				for(int k =0; k < numcoeff[i]; k++){
					//if(k==0)printf("HP %i %i %g %g \n", i, j, Betatimes[j],Hermitepoly[j][cpos+k]*exp(-0.5*Betatimes[j]*Betatimes[j]));
					double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));//*((MNStruct *)globalcontext)->Factorials[k]);
					Hermitepoly[j][cpos+k]=Hermitepoly[j][cpos+k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);

				}
					
			}
			cpos += numcoeff[i];
	   	 }

	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


		double **M = new double*[Nbins];

		int Msize = totalProfCoeff+1;
		for(int i =0; i < Nbins; i++){
			M[i] = new double[Msize];

			M[i][0] = 1;


			for(int j = 0; j < totalProfCoeff; j++){
				M[i][j+1] = Hermitepoly[i][j];
				//if(j==0)printf("%i %i %g \n", i, j, M[i][j+1]);
			}
		  
		}


		double **MNM = new double*[Msize];
		for(int i =0; i < Msize; i++){
		    MNM[i] = new double[Msize];
		}

		dgemm(M, M , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');

		for(int j = 0; j < Msize; j++){
			MNM[j][j] += pow(10.0,-14);
		}

		double minFlux = pow(10.0,100); 
		for(int j = 0; j < Nbins; j++){
			if((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] < minFlux){minFlux = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];}
		}

		double *dNM = new double[Msize];
		double *TempdNM = new double[Msize];

		double *NDiffVec = new double[Nbins];
		TemplateFlux = 0;
		for(int j = 0; j < Nbins; j++){
			TemplateFlux +=  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-minFlux;
			NDiffVec[j] = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-minFlux;
		}
		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
		}


		int info=0;
		double Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);


//		printf("Margindet %g \n", Margindet);
		double *shapevec = new double[Nbins];
		dgemv(M,TempdNM,shapevec,Nbins,Msize,'N');


		TargetSN = 10000.0;
		FakeRMS = TemplateFlux/TargetSN;
		FakeRMS = 1.0/(FakeRMS*FakeRMS);
		    double Chisq=0;
		    for(int j =0; j < Nbins; j++){


			    double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - shapevec[j]-minFlux;
			    Chisq += datadiff*datadiff*FakeRMS;
				printf("Bin %i Data %.10g Template %.10g Noise %.10g \n", j, (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] , shapevec[j], 1.0/sqrt(FakeRMS));		

		    }

		cpos = 0;
		for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
			for(int j = 0; j < numcoeff[i]; j++){
				printf("P%i %.10g \n", i, TempdNM[cpos+j+1]/TempdNM[1]);
			}
			cpos+=numcoeff[i];
		}
                printf("B %.10g \n", Cube[1]);


		profilelike = -0.5*(Chisq);
		lnew += profilelike;

		delete[] shapevec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] Betatimes;

		for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] M[j];
		}
		delete[] Hermitepoly;
		delete[] M;

		for (int j = 0; j < Msize; j++){
		    delete[] MNM[j];
		}
		delete[] MNM;
	
	
	}
	 
	 
	 
	 

	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] numcoeff;
	delete[] CompSeps;

	//printf("End Like: %.10g %g %g\n", lnew, TemplateFlux, FakeRMS);


}

/*
void SubIntStocProfLikeMNWrap(double *Cube, int &ndim, int &npars, double &lnew, void *context){



        for(int p=0;p<ndim;p++){

                Cube[p]=(((MNStruct *)globalcontext)->PriorsArray[p+ndim]-((MNStruct *)globalcontext)->PriorsArray[p])*Cube[p]+((MNStruct *)globalcontext)->PriorsArray[p];
        }


	double *DerivedParams = new double[npars];

	double result = SubIntStocProfLike(ndim, Cube, npars, DerivedParams, context);


	delete[] DerivedParams;

	lnew = result;

}



double  SubIntStocProfLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){


	struct timeval tval_before, tval_after, tval_resultone, tval_resulttwo;


//	gettimeofday(&tval_before, NULL);


	int debug = 0;

	if(debug == 1)printf("In like \n");

	double *EFAC;
	double *EQUAD;
        long double LDparams;
        int pcount;

	double uniformpriorterm = 0;


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	LDparams=Cube[0]*(((MNStruct *)globalcontext)->LDpriors[0][1]) + (((MNStruct *)globalcontext)->LDpriors[0][0]);
	
	pcount=0;
	long double phase = LDparams*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;
	
	if(debug == 1)printf("Phase: %g \n", (double)phase);


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form SATS///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	//for(int i = 0; i < 10; i ++){
	//	double pval = i*0.1;
	//	phase = pval*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;

	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase;
	}


        for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
                ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }


	fastformSubIntBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);      

	//for(int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
	//	printf("%i %g %i %.15g %.15g \n", i, pval, j, (double)((MNStruct *)globalcontext)->pulse->obsn[j].batCorr, (double)((MNStruct *)globalcontext)->pulse->obsn[j].residual	);
	//}/


	//}
	//sleep(5);

	if(debug == 1)printf("Formed Residuals \n");

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////White noise/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=1;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[0]);}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[p]);}
			pcount++;

		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
                }
        }


        double *SQUAD;
	if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
        	SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
        	for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
           		SQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
        	}
    	}	  
	  

	double HighFreqStoc1 = 0;
	double HighFreqStoc2 = 0;
	if(((MNStruct *)globalcontext)->incHighFreqStoc == 1){
		HighFreqStoc1 = pow(10.0,Cube[pcount]);
		pcount++;
	}
        if(((MNStruct *)globalcontext)->incHighFreqStoc == 2){
                HighFreqStoc1 = pow(10.0,Cube[pcount]);
                pcount++;
		HighFreqStoc2 = pow(10.0,Cube[pcount]);
                pcount++;
        }


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	      //printf("badcorr? %i %g %g \n", i,  (double)((MNStruct *)globalcontext)->pulse->obsn[i].batCorr, (double)((MNStruct *)globalcontext)->pulse->obsn[i].residual);
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr - ((MNStruct *)globalcontext)->pulse->obsn[0].batCorr;
		
	      }
	      
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr - ((MNStruct *)globalcontext)->pulse->obsn[0].batCorr - phase - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
	 }


	int maxshapecoeff = 0;
	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;
	int numshapestoccoeff = ((MNStruct *)globalcontext)->numshapestoccoeff;

	double shapecoeff[numcoeff];
	double StocProfCoeffs[numshapestoccoeff];

	for(int i =0; i < numcoeff; i++){
		shapecoeff[i]=((MNStruct *)globalcontext)->MeanProfileShape[i];
	}
	double beta = ((MNStruct *)globalcontext)->MeanProfileBeta*((MNStruct *)globalcontext)->ReferencePeriod;


	for(int i =0; i < numshapestoccoeff; i++){
		StocProfCoeffs[i]= pow(10.0, 2*Cube[pcount]);
		pcount++;
	}

	if(numcoeff+1>=numshapestoccoeff+1){
		maxshapecoeff=numcoeff+1;
	}
	if(numshapestoccoeff+1 > numcoeff+1){
		maxshapecoeff=numshapestoccoeff+1;
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	double modelflux=0;
	    for(int j =0; j < numcoeff; j=j+2){
		modelflux+=sqrt(sqrt(M_PI))*sqrt(beta)*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Binomial[j])*shapecoeff[j];
	   }

	double lnew = 0;
	int badshape = 0;

	int minoffpulse=750;
	int maxoffpulse=1250;
//        int minoffpulse=200;
//        int maxoffpulse=800;



	int GlobalNBins = (int)((MNStruct *)globalcontext)->ProfileInfo[0][1];


	double **AllHermiteBasis =  new double*[GlobalNBins];
	double **JitterBasis  =  new double*[GlobalNBins];
	double **Hermitepoly =  new double*[GlobalNBins];

	for(int i =0;i<GlobalNBins;i++){AllHermiteBasis[i]=new double[maxshapecoeff];}
	for(int i =0;i<GlobalNBins;i++){Hermitepoly[i]=new double[numcoeff];}
	for(int i =0;i<GlobalNBins;i++){JitterBasis[i]=new double[numcoeff];}


	double **M = new double*[GlobalNBins];
	double **NM = new double*[GlobalNBins];

	int Msize = 2+numshapestoccoeff;
	int Mcounter=0;
	if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
		Msize++;
	}


	for(int i =0; i < GlobalNBins; i++){
		Mcounter=0;
		M[i] = new double[Msize];
		NM[i] = new double[Msize];

	}


	double **MNM = new double*[Msize];
	for(int i =0; i < Msize; i++){
	    MNM[i] = new double[Msize];
	}


//		gettimeofday(&tval_after, NULL);

//		timersub(&tval_after, &tval_before, &tval_resultone);


//		gettimeofday(&tval_before, NULL);
	
	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){

	
//		gettimeofday(&tval_before, NULL);

		if(debug == 1)printf("In toa %i \n", t);
		int nTOA = t;

		double detN  = 0;
		double Chisq  = 0;
		double logMargindet = 0;
		double Marginlike = 0;	 
		double JitterPrior = 0;	   

		double profilelike=0;

		long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
		long double FoldingPeriodDays = FoldingPeriod/SECDAY;
		int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
		double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
		double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
		long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

		double *Betatimes = new double[Nbins];
	

	        double *shapevec  = new double[Nbins];
	        double *Jittervec = new double[Nbins];

		double noisemean=0;
//		for(int j =minoffpulse; j < maxoffpulse; j++){
//			noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
//		}

//		noisemean = noisemean/(maxoffpulse-minoffpulse);

		double MLSigma = 0;
//		int MLSigmaCount = 0;
		double dataflux = 0;
//		for(int j = 0; j < Nbins; j++){
//			if(j>=minoffpulse && j < maxoffpulse){
//				double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean;
//				MLSigma += res*res; MLSigmaCount += 1;
//			}
//			else{
//				if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean)/MLSigma > 1){
//					dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-noisemean)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
//				}
//			}
//		}
//
//		MLSigma = sqrt(MLSigma/MLSigmaCount);
		double maxamp = 0;
		double maxshape=0;
//
//		if(debug == 1)printf("noise %g, dataflux %g, modelflux %g, maxamp %g \n", MLSigma, dataflux, modelflux, maxamp);


		long double binpos = ModelBats[nTOA];

		if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

		long double minpos = binpos - FoldingPeriodDays/2;
		if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];



		int InterpBin = 0;
		double FirstInterpTimeBin = 0;
		int  NumWholeBinInterpOffset = 0;

		if(((MNStruct *)globalcontext)->InterpolateProfile == 1){

		
			long double timediff = 0;
			long double bintime = ProfileBats[t][0];


			if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
			}
			else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
			}
			else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
			}

			timediff=timediff*SECDAY;

			double OneBin = FoldingPeriod/Nbins;
			int NumBinsInTimeDiff = floor(timediff/OneBin + 0.5);
			double WholeBinsInTimeDiff = NumBinsInTimeDiff*FoldingPeriod/Nbins;
			double OneBinTimeDiff = -1*((double)timediff - WholeBinsInTimeDiff);

			double PWrappedTimeDiff = (OneBinTimeDiff - floor(OneBinTimeDiff/OneBin)*OneBin);

			if(debug == 1)printf("Making InterpBin: %g %g %i %g %g %g\n", (double)timediff, OneBin, NumBinsInTimeDiff, WholeBinsInTimeDiff, OneBinTimeDiff, PWrappedTimeDiff);

			InterpBin = floor(PWrappedTimeDiff/((MNStruct *)globalcontext)->InterpolatedTime+0.5);
			if(InterpBin >= ((MNStruct *)globalcontext)->NumToInterpolate)InterpBin -= ((MNStruct *)globalcontext)->NumToInterpolate;

			FirstInterpTimeBin = -1*(InterpBin-1)*((MNStruct *)globalcontext)->InterpolatedTime;

			if(debug == 1)printf("Interp Time Diffs: %g %g %g %g \n", ((MNStruct *)globalcontext)->InterpolatedTime, InterpBin*((MNStruct *)globalcontext)->InterpolatedTime, PWrappedTimeDiff, InterpBin*((MNStruct *)globalcontext)->InterpolatedTime-PWrappedTimeDiff);

			double FirstBinOffset = timediff-FirstInterpTimeBin;
			double dNumWholeBinOffset = FirstBinOffset/(FoldingPeriod/Nbins);
			int  NumWholeBinOffset = 0;

			NumWholeBinInterpOffset = floor(dNumWholeBinOffset+0.5);
	
			if(debug == 1)printf("Interp bin is: %i , First Bin is %g, Offset is %i \n", InterpBin, FirstInterpTimeBin, NumWholeBinInterpOffset);


		}
	   
		if(((MNStruct *)globalcontext)->InterpolateProfile == 0){

 
			for(int j =0; j < Nbins; j++){


				long double timediff = 0;
				long double bintime = ProfileBats[t][j];
					

				if(bintime  >= minpos && bintime <= maxpos){
				    timediff = bintime - binpos;
				}
				else if(bintime < minpos){
				    timediff = FoldingPeriodDays+bintime - binpos;
				}
				else if(bintime > maxpos){
				    timediff = bintime - FoldingPeriodDays - binpos;
				}


				timediff=timediff*SECDAY;


				Betatimes[j]=timediff/beta;
				TNothpl(maxshapecoeff,Betatimes[j],AllHermiteBasis[j]);

				for(int k =0; k < maxshapecoeff; k++){
					double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));
					AllHermiteBasis[j][k]=AllHermiteBasis[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);

					if(k<numcoeff){ Hermitepoly[j][k] = AllHermiteBasis[j][k];}


			//		if(((MNStruct *)globalcontext)->InterpolateProfile == 1 && k == 0){	
			//			printf("Interped: %i %.10g %.10g %.10g \n", j, (double)timediff,AllHermiteBasis[j][k], ((MNStruct *)globalcontext)->InterpolatedShapelets[InterpBin][Nj][k]); 
			//		}
				}

				JitterBasis[j][0] = (1.0/sqrt(2.0))*(-1.0*Hermitepoly[j][1]);
				for(int k =1; k < numcoeff; k++){
					JitterBasis[j][k] = (1.0/sqrt(2.0))*(sqrt(double(k))*AllHermiteBasis[j][k-1] - sqrt(double(k+1))*AllHermiteBasis[j][k+1]);
				}

			}

			dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');
			dgemv(JitterBasis,shapecoeff,Jittervec,Nbins,numcoeff,'N');


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////Get Noise Level//////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



			maxshape=0;
			for(int j =0; j < Nbins; j++){
				if(shapevec[j] > maxshape){ maxshape = shapevec[j];}
			}
			noisemean=0;
			int numoffpulse = 0;
			for(int j = 0; j < Nbins; j++){
				if(shapevec[j] < maxshape*0.001){
					noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
					numoffpulse += 1;
				}	
			}


			noisemean = noisemean/(numoffpulse);

			MLSigma = 0;
			int MLSigmaCount = 0;
			dataflux = 0;
			for(int j = 0; j < Nbins; j++){
				if(shapevec[j] < maxshape*0.001){
					double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean;
					MLSigma += res*res; MLSigmaCount += 1;
				}
				else{
					if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean)/MLSigma > 1){
						dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-noisemean)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
					}
				}
			}

			MLSigma = sqrt(MLSigma/MLSigmaCount);
			MLSigma = MLSigma*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];
			maxamp = dataflux/modelflux;
			if(debug == 1)printf("noise %g, dataflux %g, modelflux %g, maxamp %g \n", MLSigma, dataflux, modelflux, maxamp);

                        for(int j =0; j < Nbins; j++){

				
				M[j][0] = 1;
				M[j][1] = shapevec[j];

				Mcounter = 2;

				if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
					M[j][Mcounter] = Jittervec[j]*dataflux/modelflux/beta;
					Mcounter++;
				}

				for(int k = 0; k < numshapestoccoeff; k++){
				    M[j][Mcounter+k] = AllHermiteBasis[j][k]*dataflux;
				}

			}
		}

		if(((MNStruct *)globalcontext)->InterpolateProfile == 1){


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////Fill Arrays with interpolated state//////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			for(int j =0; j < Nbins; j++){

				double NewIndex = (j + NumWholeBinInterpOffset);
				int Nj = (int)(NewIndex - floor(NewIndex/Nbins)*Nbins);

				M[j][0] = 1;
				M[j][1] = ((MNStruct *)globalcontext)->InterpolatedMeanProfile[InterpBin][Nj];

				Mcounter = 2;
				
				if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
					M[j][Mcounter] = ((MNStruct *)globalcontext)->InterpolatedJitterProfile[InterpBin][Nj];
					Mcounter++;
				}			

				for(int k = 0; k < numshapestoccoeff; k++){
				  //  M[j][Mcounter+k] = ((MNStruct *)globalcontext)->InterpolatedShapelets[InterpBin][Nj][k];
				}

				shapevec[j] = ((MNStruct *)globalcontext)->InterpolatedMeanProfile[InterpBin][Nj];
			}



		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////Get Noise Level//////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			noisemean=0;
			int numoffpulse = 0;
                        maxshape=((MNStruct *)globalcontext)->MaxShapeAmp;

			for(int j = 0; j < Nbins; j++){
				if(shapevec[j] < ((MNStruct *)globalcontext)->MaxShapeAmp*0.001){
					noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
					numoffpulse += 1;
				}	
			}


			noisemean = noisemean/(numoffpulse);

			for(int j =minoffpulse; j < maxoffpulse; j++){
				noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
			}
	                noisemean = noisemean/(maxoffpulse-minoffpulse);

			MLSigma = 0;
			int MLSigmaCount = 0;
			dataflux = 0;
			for(int j = 0; j < Nbins; j++){
				if(shapevec[j] < ((MNStruct *)globalcontext)->MaxShapeAmp*0.001){
					double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean;
					MLSigma += res*res; MLSigmaCount += 1;
				}
			//	else{
			//		if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean)/MLSigma > 1){
			//			dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-noisemean)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
			//		}
			//	}
			}


		MLSigma = 0;
		MLSigmaCount = 0;
		dataflux = 0;
		for(int j = 0; j < Nbins; j++){
			if(j>=minoffpulse && j < maxoffpulse){
				double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] -noisemean;
				MLSigma += res*res; MLSigmaCount += 1;
			}
			else{
				if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean)/MLSigma > 1){
					dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-noisemean)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
				}
			}
		}

			MLSigma = sqrt(MLSigma/MLSigmaCount);
			MLSigma = MLSigma*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];

			dataflux = 0;
			for(int j = 0; j < Nbins; j++){
//				printf("shapevec %i %g %g %g \n", j, shapevec[j], ((MNStruct *)globalcontext)->MaxShapeAmp, ((MNStruct *)globalcontext)->MaxShapeAmp*0.001);
				if(shapevec[j] < ((MNStruct *)globalcontext)->MaxShapeAmp*0.001){
//					if(t==0){printf("in %i \n", j);}
				}
				else{
					if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean)/MLSigma > 1){
						dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-noisemean)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
					}
				}
			}

			maxamp = dataflux/modelflux;
			//if(t==0){printf("prof %i MLSigmaCount %i, noise %g, dataflux %g, modelflux %g, maxamp %g noisemean %g\n", t, MLSigmaCount, MLSigma, dataflux, modelflux, maxamp, noisemean);}
			if(debug == 1)printf("noise %g, dataflux %g, modelflux %g, maxamp %g \n", MLSigma, dataflux, modelflux, maxamp);


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////Rescale Basis Vectors////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			for(int j =0; j < Nbins; j++){

				Mcounter = 2;
				
				if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
					M[j][Mcounter] = M[j][Mcounter]*dataflux/modelflux/beta;
					Mcounter++;
				}			

				for(int k = 0; k < numshapestoccoeff; k++){
				    M[j][Mcounter+k] = M[j][Mcounter+k]*dataflux;
				}
			}
	    }

	
	    double *NDiffVec = new double[Nbins];

//	    double maxshape=0;
//	    for(int j =0; j < Nbins; j++){
// 	        if(shapevec[j] > maxshape){ maxshape = shapevec[j];}
//	    }


	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


		      
		Chisq = 0;
		detN = 0;

	
 
		double highfreqnoise = HighFreqStoc1;
		double flatnoise = HighFreqStoc2;


		for(int i =0; i < Nbins; i++){
		  	Mcounter=2;
			double noise = MLSigma*MLSigma +  pow(highfreqnoise*maxamp*shapevec[i],2);

			if(shapevec[i] > maxshape*0.001){
				noise +=  pow(flatnoise*maxamp*maxshape,2);
			}
			detN += log(noise);

			double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1];

			NDiffVec[i] = datadiff/(noise);
		       

			Chisq += datadiff*datadiff/(noise);
	


			for(int j = 0; j < Msize; j++){
				NM[i][j] = M[i][j]/(noise);
			}


		}
	
	
	//	gettimeofday(&tval_after, NULL);

	//	timersub(&tval_after, &tval_before, &tval_resultone);


	//	gettimeofday(&tval_before, NULL);
			
		dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');


		Mcounter=2;
		double JitterDet = 0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
			double profnoise = EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
			profnoise = profnoise*profnoise;
		    	JitterDet = log(profnoise);
		    	MNM[2][2] += 1.0/profnoise;
			Mcounter++;
		}


		double StocProfDet = 0;
		for(int j = 0; j < numshapestoccoeff; j++){
			StocProfDet += log(StocProfCoeffs[j]);
			MNM[Mcounter+j][Mcounter+j] +=  1.0/StocProfCoeffs[j];
		}

		double *dNM = new double[Msize];
		double *TempdNM = new double[Msize];
		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
		}


		int info=0;
		double Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);
			    
		logMargindet = Margindet;

		Marginlike = 0;
		for(int i =0; i < Msize; i++){
			Marginlike += TempdNM[i]*dNM[i];
		}

		profilelike = -0.5*(detN + Chisq + logMargindet + StocProfDet + JitterDet - Marginlike);
		lnew += profilelike;
		if(debug == 1)printf("Like: %i %.15g %.15g %.15g %.15g %.15g %.15g %.15g\n", nTOA,lnew, detN, Chisq, logMargindet, Marginlike, StocProfDet , JitterDet);

		delete[] shapevec;
		delete[] Jittervec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] TempdNM;
		delete[] Betatimes;


	
//        	gettimeofday(&tval_after, NULL);

  //      	timersub(&tval_after, &tval_before, &tval_resulttwo);
    //    	printf("Time elapsed: %ld.%06ld %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec, (long int)tval_resulttwo.tv_sec, (long int)tval_resulttwo.tv_usec);
	
	}
	 
	 
	for (int j = 0; j < GlobalNBins; j++){
	    delete[] Hermitepoly[j];
	    delete[] JitterBasis[j];
	    delete[] AllHermiteBasis[j];
	    delete[] M[j];
	    delete[] NM[j];
	}
	delete[] Hermitepoly;
	delete[] AllHermiteBasis;
	delete[] JitterBasis;
	delete[] M;
	delete[] NM;

	for (int j = 0; j < Msize; j++){
	    delete[] MNM[j];
	}
	delete[] MNM;
	 

	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;


	lnew += uniformpriorterm;
	if(debug == 1)printf("End Like: %.10g \n", lnew);
	//printf("End Like: %.10g \n", lnew);
	//}


       	//gettimeofday(&tval_after, NULL);

       	//timersub(&tval_after, &tval_before, &tval_resulttwo);
       	//printf("Time elapsed: %ld.%06ld %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec, (long int)tval_resulttwo.tv_sec, (long int)tval_resulttwo.tv_usec);
	
	return lnew;
	//sleep(5);
	//return bluff;
	//sleep(5);

}

void  WriteSubIntStocProfLike(std::string longname, int &ndim){



        double *Cube = new double[ndim];
        int number_of_lines = 0;

        std::ifstream checkfile;
	std::string checkname;
	if(((MNStruct *)globalcontext)->sampler == 0){
	        checkname = longname+"phys_live.points";
	}
        if(((MNStruct *)globalcontext)->sampler == 1){
                checkname = longname+"_phys_live.txt";
        }
	//printf("%i %s \n", ((MNStruct *)globalcontext)->sampler, checkname.c_str());
        checkfile.open(checkname.c_str());
        std::string line;
        while (getline(checkfile, line))
                ++number_of_lines;

        checkfile.close();

        std::ifstream summaryfile;
	std::string fname;
	if(((MNStruct *)globalcontext)->sampler == 0){
		fname = longname+"phys_live.points";
	}
	if(((MNStruct *)globalcontext)->sampler == 1){
	        fname = longname+"_phys_live.txt";
	}

        summaryfile.open(fname.c_str());



        printf("Getting ML \n");
        double maxlike = -1.0*pow(10.0,10);
        for(int i=0;i<number_of_lines;i++){

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);
                double like = paramlist[ndim];

                if(like > maxlike){
                        maxlike = like;
                         for(int i = 0; i < ndim; i++){
                                 Cube[i] = paramlist[i];
                         }
                }

        }
        summaryfile.close();
	printf("ML val: %.10g \n", maxlike);

	int debug = 0;

	if(debug == 1)printf("In like \n");

	double *EFAC;
	double *EQUAD;
	long double LDparams;
	int pcount;

	double uniformpriorterm = 0;


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	LDparams=Cube[0]*(((MNStruct *)globalcontext)->LDpriors[0][1]) + (((MNStruct *)globalcontext)->LDpriors[0][0]);
	
	pcount=0;
	long double phase = LDparams*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;
	
	if(debug == 1)printf("Phase: %g \n", (double)phase);


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form SATS///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase;
	}


	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
	}


	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);       


	if(debug == 1)printf("Formed Residuals \n");

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////White noise/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[0]);}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[p]);}
			pcount++;
		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
		}
	}


	double *SQUAD;
	if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
		}
	}	  
	  
	double HighFreqStoc1 = 0;
	double HighFreqStoc2 = 0;
	if(((MNStruct *)globalcontext)->incHighFreqStoc == 1){
		HighFreqStoc1 = pow(10.0,Cube[pcount]);
		pcount++;
	}
        if(((MNStruct *)globalcontext)->incHighFreqStoc == 2){
                HighFreqStoc1 = pow(10.0,Cube[pcount]);
                pcount++;
		HighFreqStoc2 = pow(10.0,Cube[pcount]);
                pcount++;
        }
	//printf("End of White\n");


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	      //printf("badcorr? %i %g %g \n", i,  (double)((MNStruct *)globalcontext)->pulse->obsn[i].batCorr, (double)((MNStruct *)globalcontext)->pulse->obsn[i].residual);
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr - ((MNStruct *)globalcontext)->pulse->obsn[0].batCorr;
		
	      }
	      
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr - ((MNStruct *)globalcontext)->pulse->obsn[0].batCorr - phase - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
	 }


	int maxshapecoeff = 0;
	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;
	int numshapestoccoeff = ((MNStruct *)globalcontext)->numshapestoccoeff;

	double shapecoeff[numcoeff];
	double StocProfCoeffs[numshapestoccoeff];

	for(int i =0; i < numcoeff; i++){
		shapecoeff[i]=((MNStruct *)globalcontext)->MeanProfileShape[i];
	}
	double beta = ((MNStruct *)globalcontext)->MeanProfileBeta*((MNStruct *)globalcontext)->ReferencePeriod;


	for(int i =0; i < numshapestoccoeff; i++){
		StocProfCoeffs[i]= pow(10.0, 2*Cube[pcount]);
		pcount++;
	}

	if(numcoeff+1>=numshapestoccoeff+1){
		maxshapecoeff=numcoeff+1;
	}
	if(numshapestoccoeff+1 > numcoeff+1){
		maxshapecoeff=numshapestoccoeff+1;
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	double modelflux=0;
	    for(int j =0; j < numcoeff; j=j+2){
		modelflux+=sqrt(sqrt(M_PI))*sqrt(beta)*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Binomial[j])*shapecoeff[j];
	   }

	double lnew = 0;
	int badshape = 0;

//	int minoffpulse=750;
//	int maxoffpulse=1250;
       // int minoffpulse=200;
       // int maxoffpulse=800;
	
	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
		if(debug == 1)printf("In toa %i \n", t);
		int nTOA = t;

		double detN  = 0;
		double Chisq  = 0;
		double logMargindet = 0;
		double Marginlike = 0;	 
		double JitterPrior = 0;	   

		double profilelike=0;

		long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
		long double FoldingPeriodDays = FoldingPeriod/SECDAY;
		int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
		double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
		double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
		long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

		double *Betatimes = new double[Nbins];
		double **AllHermiteBasis =  new double*[Nbins];
		double **JitterBasis  =  new double*[Nbins];
		double **Hermitepoly =  new double*[Nbins];

		for(int i =0;i<Nbins;i++){AllHermiteBasis[i]=new double[maxshapecoeff];for(int j =0;j<maxshapecoeff;j++){AllHermiteBasis[i][j]=0;}}
		for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}
		for(int i =0;i<Nbins;i++){JitterBasis[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){JitterBasis[i][j]=0;}}

	
	

		long double binpos = ModelBats[nTOA];


		if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

		long double minpos = binpos - FoldingPeriodDays/2;
		if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];


	    
		for(int j =0; j < Nbins; j++){
			long double timediff = 0;
			long double bintime = ProfileBats[t][j];
			if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
			}
			else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
			}
			else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
			}

			timediff=timediff*SECDAY;

			Betatimes[j]=timediff/beta;
			TNothpl(maxshapecoeff,Betatimes[j],AllHermiteBasis[j]);

			for(int k =0; k < maxshapecoeff; k++){
				double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));
				AllHermiteBasis[j][k]=AllHermiteBasis[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);

				if(k<numcoeff){ Hermitepoly[j][k] = AllHermiteBasis[j][k];}
			
			}

			JitterBasis[j][0] = (1.0/sqrt(2.0))*(-1.0*AllHermiteBasis[j][1]);
			for(int k =1; k < numcoeff; k++){
				JitterBasis[j][k] = (1.0/sqrt(2.0))*(sqrt(double(k))*AllHermiteBasis[j][k-1] - sqrt(double(k+1))*AllHermiteBasis[j][k+1]);
			}
			
	
	    }


		double *shapevec  = new double[Nbins];
		double *Jittervec = new double[Nbins];

		double OverallProfileAmp = shapecoeff[0];

		shapecoeff[0] = 1;

		dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');
		dgemv(JitterBasis,shapecoeff,Jittervec,Nbins,numcoeff,'N');

		double *NDiffVec = new double[Nbins];

		double maxshape=0;


		for(int j =0; j < Nbins; j++){

		    if(shapevec[j] > maxshape){ maxshape = shapevec[j];}

		}




			double noisemean=0;
			int numoffpulse = 0;
			for(int j = 0; j < Nbins; j++){
				if(shapevec[j] < maxshape*0.001){
					noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
					numoffpulse += 1;
				}	
			}


			noisemean = noisemean/(numoffpulse);

			double MLSigma = 0;
			int MLSigmaCount = 0;
			double dataflux = 0;
			for(int j = 0; j < Nbins; j++){
				if(shapevec[j] < maxshape*0.001){
					double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean;
					MLSigma += res*res; MLSigmaCount += 1;
				}
				else{
					if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean)/MLSigma > 1){
						dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-noisemean)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
					}
				}
			}

			MLSigma = sqrt(MLSigma/MLSigmaCount);
			MLSigma = MLSigma*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];
			double maxamp = dataflux/modelflux;
			if(debug == 1)printf("noise %g, dataflux %g, modelflux %g, maxamp %g \n", MLSigma, dataflux, modelflux, maxamp);


	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


		double **M = new double*[Nbins];
		double **NM = new double*[Nbins];

		int Msize = 2+numshapestoccoeff;
		int Mcounter=0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
			Msize++;
		}


		for(int i =0; i < Nbins; i++){
			Mcounter=0;
			M[i] = new double[Msize];
			NM[i] = new double[Msize];

			M[i][0] = 1;
			M[i][1] = shapevec[i];

			Mcounter = 2;

			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
				M[i][Mcounter] = Jittervec[i];
				Mcounter++;
			}			

			for(int j = 0; j < numshapestoccoeff; j++){
			    M[i][Mcounter+j] = AllHermiteBasis[i][j];
			}
		}


		double **MNM = new double*[Msize];
		for(int i =0; i < Msize; i++){
		    MNM[i] = new double[Msize];
		}

		      
		Chisq = 0;
		detN = 0;

	
 		double highfreqnoise = HighFreqStoc1;
		double flatnoise = HighFreqStoc2;
		
		double *profilenoise = new double[Nbins];

		for(int i =0; i < Nbins; i++){
			Mcounter=2;
			double noise = MLSigma*MLSigma +  pow(highfreqnoise*maxamp*shapevec[i],2);
			if(shapevec[i] > maxshape*0.001){
				noise +=  pow(flatnoise*maxamp*maxshape,2);
			}
			detN += log(noise);
			profilenoise[i] = sqrt(noise);

			double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1];

			NDiffVec[i] = datadiff/(noise);
		       

			Chisq += datadiff*datadiff/(noise);
	
			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
				M[i][2] = M[i][2]*dataflux/modelflux/beta;
				Mcounter++;
			}
			
			for(int j = 0; j < numshapestoccoeff; j++){
				M[i][Mcounter+j] = M[i][Mcounter+j]*dataflux;

			}


			for(int j = 0; j < Msize; j++){
				NM[i][j] = M[i][j]/(noise);
			}


		}
		
			
		dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');


		Mcounter=2;
		double JitterDet = 0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
			double profnoise = EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
			profnoise = profnoise*profnoise;
			JitterDet = log(profnoise);
			MNM[2][2] += 1.0/profnoise;
			Mcounter++;
		}


		double StocProfDet = 0;
		for(int j = 0; j < numshapestoccoeff; j++){
			StocProfDet += log(StocProfCoeffs[j]);
			MNM[Mcounter+j][Mcounter+j] +=  1.0/StocProfCoeffs[j];
		}

		double *dNM = new double[Msize];
		double *TempdNM = new double[Msize];
		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
		}


		int info=0;
		double Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);
			    
		logMargindet = Margindet;

		Marginlike = 0;
		for(int i =0; i < Msize; i++){
			Marginlike += TempdNM[i]*dNM[i];
		}


		double finaloffset = TempdNM[0];
		double finalamp = TempdNM[1];
		double finalJitter = 0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0)finalJitter = TempdNM[2];

		double *StocVec = new double[Nbins];
		
		for(int i =0; i < Nbins; i++){
			if(((MNStruct *)globalcontext)->numFitEQUAD > 0)Jittervec[i] = M[i][2];
			if(((MNStruct *)globalcontext)->numFitEQUAD == 0)Jittervec[i] = 0;
		}

		dgemv(M,TempdNM,shapevec,Nbins,Msize,'N');

		

		TempdNM[0] = 0;
		TempdNM[1] = 0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0)TempdNM[2] = 0;
		

		dgemv(M,TempdNM,StocVec,Nbins,Msize,'N');

		std::ofstream profilefile;
		char toanum[15];
		sprintf(toanum, "%d", nTOA);
		std::string ProfileName =  toanum;
		std::string dname = longname+ProfileName+"-Profile.txt";
	
		profilefile.open(dname.c_str());
		double MLChisq = 0;
		for(int i =0; i < Nbins; i++){
			MLChisq += pow(((double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1] - shapevec[i])/profilenoise[i], 2);
			profilefile << i << " " << std::setprecision(10) << (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1] << " " << shapevec[i] << " " << profilenoise[i] << " " << finaloffset + finalamp*M[i][1] << " " << finalJitter*Jittervec[i] << " " << StocVec[i] << "\n";

		}
	    	profilefile.close();
		delete[] profilenoise;
		delete[] StocVec;
		
		profilelike = -0.5*(detN + Chisq + logMargindet + StocProfDet + JitterDet - Marginlike);
		lnew += profilelike;

		printf("Profile chisq and like: %g %g \n", MLChisq, profilelike);
//		printf("Like: %i %.15g %.15g %.15g %.15g %.15g %.15g %.15g\n", nTOA,lnew, detN, Chisq, logMargindet, Marginlike, StocProfDet , JitterDet);

		delete[] shapevec;
		delete[] Jittervec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] Betatimes;

		for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] JitterBasis[j];
		    delete[] AllHermiteBasis[j];
		    delete[] M[j];
		    delete[] NM[j];
		}
		delete[] Hermitepoly;
		delete[] AllHermiteBasis;
		delete[] JitterBasis;
		delete[] M;
		delete[] NM;

		for (int j = 0; j < Msize; j++){
		    delete[] MNM[j];
		}
		delete[] MNM;
	
	
	}
	 
	 
	 
	 

	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;


	lnew += uniformpriorterm;
	printf("End Like: %.10g \n", lnew);


}
*/


void PreComputeShapelets(double **StoredShapelets, double **StoredJitterShapelets, double **InterpolatedMeanProfile, double **InterpolatedJitterProfile, double **InterpolatedWidthProfile, long double finalInterpTime, int numtointerpolate, double MeanBeta, double &MaxShapeAmp){


	printf("In function %Lg %i %g \n", finalInterpTime, numtointerpolate, MeanBeta);

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	int maxshapecoeff = 0;
	int totshapecoeff = ((MNStruct *)globalcontext)->totshapecoeff;

	int *numcoeff= new int[((MNStruct *)globalcontext)->numProfComponents];

	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		numcoeff[i] =  ((MNStruct *)globalcontext)->numshapecoeff[i];
	}

	int totalshapestoccoeff = ((MNStruct *)globalcontext)->totalshapestoccoeff;
	int *numProfileStocCoeff = ((MNStruct *)globalcontext)->numshapestoccoeff;

	int *numEvoCoeff = ((MNStruct *)globalcontext)->numEvoCoeff;
	int totalEvoCoeff = ((MNStruct *)globalcontext)->totalEvoCoeff;

	int *numProfileFitCoeff = ((MNStruct *)globalcontext)->numProfileFitCoeff;	
	int totalProfileFitCoeff = ((MNStruct *)globalcontext)->totalProfileFitCoeff;

        int *numShapeToSave = new int[((MNStruct *)globalcontext)->numProfComponents];
        for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
                numShapeToSave[i] = numProfileStocCoeff[i];
		if(numEvoCoeff[i] >  numShapeToSave[i]){numShapeToSave[i] = numEvoCoeff[i];}
		if(numProfileFitCoeff[i] >  numShapeToSave[i]){numShapeToSave[i] = numProfileFitCoeff[i];}
		printf("saving %i %i \n", i, numShapeToSave[i]);
        }
	int totShapeToSave = 0;
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		totShapeToSave += numShapeToSave[i];
	}


	double shapecoeff[totshapecoeff];

	for(int i =0; i < totshapecoeff; i++){
		shapecoeff[i]=((MNStruct *)globalcontext)->MeanProfileShape[i];
		//printf("ShapeCoeff: %i %g \n", i, shapecoeff[i]);
	}

	double beta = MeanBeta*((MNStruct *)globalcontext)->ReferencePeriod;

	maxshapecoeff = totshapecoeff +  ((MNStruct *)globalcontext)->numProfComponents;

	long double *CompSeps = new long double[((MNStruct *)globalcontext)->numProfComponents];
	CompSeps[0] = 0;
	for(int i = 1; i < ((MNStruct *)globalcontext)->numProfComponents; i++){

//		CompSeps[i] = 0.075676881*((MNStruct *)globalcontext)->ReferencePeriod; //Sim
		CompSeps[i] = 0.336277178806697163*((MNStruct *)globalcontext)->ReferencePeriod; //J1744


		//printf("CompSep: %g \n", Cube[pcount-1]);
	}


/*
	if(totshapecoeff+1>=numshapestoccoeff+1){
		maxshapecoeff=totshapecoeff+1;
	}
	if(numshapestoccoeff+1 > totshapecoeff+1){
		maxshapecoeff=numshapestoccoeff+1;
	}
*/
/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[0][1];
	long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;
	long double FoldingPeriodDays = ReferencePeriod/SECDAY;
	double maxshapeamp = 0;

	int JitterProfComp = 0;
		
	if(((MNStruct *)globalcontext)->JitterProfComp > 0){
		JitterProfComp = ((MNStruct *)globalcontext)->JitterProfComp-1;
	}

	printf("Computing %i Interpolated Profiles \n", numtointerpolate);
	for(int t = 0; t < numtointerpolate; t++){



		double **WidthBasis  =  new double*[Nbins];
		double **JitterBasis  =  new double*[Nbins];
		double **Hermitepoly =  new double*[Nbins];
		double **AllHermiteBasis = new double*[Nbins];

                for(int i =0;i<Nbins;i++){AllHermiteBasis[i]=new double[maxshapecoeff];for(int j =0;j<maxshapecoeff;j++){AllHermiteBasis[i][j]=0;}}
		for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[totshapecoeff];for(int j =0;j<totshapecoeff;j++){Hermitepoly[i][j]=0;}}
		for(int i =0;i<Nbins;i++){JitterBasis[i]=new double[totshapecoeff];for(int j =0;j<totshapecoeff;j++){JitterBasis[i][j]=0;}}
		for(int i =0;i<Nbins;i++){WidthBasis[i]=new double[totshapecoeff];for(int j =0;j<totshapecoeff;j++){WidthBasis[i][j]=0;}}

		long double interpStep = finalInterpTime;
		long double binpos = t*interpStep/SECDAY;

//		printf("Interp step %i %.10Lg %.10Lg \n", t, ReferencePeriod/Nbins, binpos);
		
		long double minpos = binpos - FoldingPeriodDays/2;
		if(minpos < 0 ) minpos= 0;
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos > FoldingPeriodDays)maxpos = FoldingPeriodDays;
	    
		int cpos = 0;
		int jpos=0;
		int spos = 0;
		for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
	
			//double oneAndThree = 0;
	
			for(int j =0; j < Nbins; j++){
				long double timediff = 0;
				long double bintime = j*FoldingPeriodDays/Nbins+CompSeps[i]/SECDAY;
				if(bintime  >= minpos && bintime <= maxpos){
				    timediff = bintime - binpos;
				}
				else if(bintime < minpos){
				    timediff = FoldingPeriodDays+bintime - binpos;
				}
				else if(bintime > maxpos){
				    timediff = bintime - FoldingPeriodDays - binpos;
				}

				timediff=timediff*SECDAY;
				double Betatime = timediff/beta;
				TNothplMC(numcoeff[i]+1,Betatime,AllHermiteBasis[j], jpos);

			
				

				for(int k =0; k < numcoeff[i]+1; k++){
					double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));
					AllHermiteBasis[j][k+jpos] = AllHermiteBasis[j][k+jpos]*Bconst*exp(-0.5*Betatime*Betatime);
		//			if(t == 0 && k == 0)printf("%i %i %g \n", i, j, AllHermiteBasis[j][k+jpos]);

					if(k<numcoeff[i]){ Hermitepoly[j][k+cpos] = AllHermiteBasis[j][k+jpos]; }
				}

				//oneAndThree +=  Hermitepoly[j][1+cpos]*Hermitepoly[j][3+cpos];


				if(((MNStruct *)globalcontext)->JitterProfComp == 0){
					JitterBasis[j][0+cpos] = (1.0/sqrt(2.0))*(-1.0*AllHermiteBasis[j][1+jpos]);
					for(int k =1; k < numcoeff[i]; k++){
						JitterBasis[j][k+cpos] = (1.0/sqrt(2.0))*(sqrt(double(k))*AllHermiteBasis[j][k-1+jpos] - sqrt(double(k+1))*AllHermiteBasis[j][k+1+jpos]);
					}

					WidthBasis[j][0+cpos] = (Betatime*Betatime - 0.5)*AllHermiteBasis[j][0+jpos];
					for(int k =1; k < numcoeff[i]; k++){
						WidthBasis[j][k+cpos] = ((Betatime*Betatime - 0.5)*AllHermiteBasis[j][k+jpos] - sqrt(double(2*k))*Betatime*AllHermiteBasis[j][k-1+jpos]);
					}
				}
				else{
					if(i == ((MNStruct *)globalcontext)->JitterProfComp - 1){
						JitterBasis[j][0+cpos] = (1.0/sqrt(2.0))*(-1.0*AllHermiteBasis[j][1+jpos]);
						for(int k =1; k < numcoeff[i]; k++){
							JitterBasis[j][k+cpos] = (1.0/sqrt(2.0))*(sqrt(double(k))*AllHermiteBasis[j][k-1+jpos] - sqrt(double(k+1))*AllHermiteBasis[j][k+1+jpos]);
						}

						WidthBasis[j][0+cpos] = (Betatime*Betatime - 0.5)*AllHermiteBasis[j][0+jpos];
						for(int k =1; k < numcoeff[i]; k++){
							WidthBasis[j][k+cpos] = ((Betatime*Betatime - 0.5)*AllHermiteBasis[j][k+jpos] - sqrt(double(2*k))*Betatime*AllHermiteBasis[j][k-1+jpos]);
						}

					}
				}
				for(int k = 0; k < numShapeToSave[i]; k++){
					//if(t==0){printf("Save: %i %i %i %g \n", k, j, spos, AllHermiteBasis[j][k+jpos]);}
					StoredShapelets[t][j+(k+spos)*Nbins] = AllHermiteBasis[j][k+jpos];
					StoredJitterShapelets[t][j+(k+spos)*Nbins] = JitterBasis[j][k+cpos];
				
				}	
		    	}

			//if(oneAndThree > pow(10.0, -10)){printf("One and Three: %i %g \n", t, oneAndThree);}

			spos += numShapeToSave[i];
			cpos += numcoeff[i];
			jpos += numcoeff[i]+1;
		}

		double *shapevec  = new double[Nbins];
		double *Jittervec = new double[Nbins];
		double *Widthvec = new double[Nbins];

		dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,totshapecoeff,'N');
		dgemv(JitterBasis,shapecoeff,Jittervec,Nbins,totshapecoeff,'N');
		dgemv(WidthBasis,shapecoeff,Widthvec,Nbins,totshapecoeff,'N');

		for(int j =0; j < Nbins; j++){
			InterpolatedMeanProfile[t][j] = shapevec[j];
			InterpolatedJitterProfile[t][j] = Jittervec[j];
			InterpolatedWidthProfile[t][j] = Widthvec[j];
			if(fabs(shapevec[j]) > maxshapeamp){ maxshapeamp = fabs(shapevec[j]); }

			//if(t==0){printf("Means: %i %g %g %g \n", j, shapevec[j],Jittervec[j], Widthvec[j]);}
		}	

		delete[] shapevec;
		delete[] Jittervec;
		delete[] Widthvec;


		for (int j = 0; j < Nbins; j++){
		    delete[] AllHermiteBasis[j]; 
		    delete[] Hermitepoly[j];
		    delete[] JitterBasis[j];
		    delete[] WidthBasis[j];
		}
		delete[] AllHermiteBasis;
		delete[] Hermitepoly;
		delete[] JitterBasis;	
		delete[] WidthBasis;

	}
	printf("Finished Computing Interpolated Profiles, max amp is %g \n", maxshapeamp);
	MaxShapeAmp = maxshapeamp;

	delete[] numcoeff;
	delete[] CompSeps;	
}


void ProfileDomainLikeMNWrap(double *Cube, int &ndim, int &npars, double &lnew, void *context){



        for(int p=0;p<ndim;p++){

                Cube[p]=(((MNStruct *)globalcontext)->PriorsArray[p+ndim]-((MNStruct *)globalcontext)->PriorsArray[p])*Cube[p]+((MNStruct *)globalcontext)->PriorsArray[p];
        }


	double *DerivedParams = new double[npars];

	double result = ProfileDomainLike(ndim, Cube, npars, DerivedParams, context);


	delete[] DerivedParams;

	lnew = result;

}

double  ProfileDomainLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){

	int dotime = 0;
	int debug = ((MNStruct *)globalcontext)->debug; 


	struct timeval tval_before, tval_after, tval_resultone, tval_resulttwo;

	if(dotime == 1){
		gettimeofday(&tval_before, NULL);
	}

	//double bluff = 0;
	//for(int loop = 0; loop < 5; loop++){
	//	Cube[0] = loop*0.2;

	
	if(debug == 1)printf("In like \n");

	double *EFAC;
	double *EQUAD;
        long double LDparams[ndim];
        int pcount;

	double uniformpriorterm = 0;



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){

		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

	}	
	
	
	pcount=0;
	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
           	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
                	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){
                        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
			}
                 }
           }

	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form SATS///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}

	///for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
	//	((MNStruct *)globalcontext)->pulse->jumpVal[k]= 0;
	//}
       for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
              ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }

	delete[] JumpVec;


	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       /* Form Barycentric arrival times */
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);       /* Form residuals */
	



	
	if(debug == 1)printf("Phase: %g \n", (double)phase);
	if(debug == 1)printf("Formed Residuals \n");

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////White noise/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=1;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[0]);}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[p]);}
			pcount++;

		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		if(((MNStruct *)globalcontext)->EQUADPriorType ==1) {uniformpriorterm += log(EQUAD[0]);}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			if(((MNStruct *)globalcontext)->EQUADPriorType ==1) {uniformpriorterm += log(EQUAD[o]);}
			pcount++;
                }
        }


        double *SQUAD;
	if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
        	SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
        	for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
           		SQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
        	}
    	}	  
	  

	double HighFreqStoc1 = 0;
	double HighFreqStoc2 = 0;
	if(((MNStruct *)globalcontext)->incHighFreqStoc == 1){
		HighFreqStoc1 = pow(10.0,Cube[pcount]);
		pcount++;
	}
        if(((MNStruct *)globalcontext)->incHighFreqStoc == 2){
                HighFreqStoc1 = pow(10.0,Cube[pcount]);
                pcount++;
		HighFreqStoc2 = pow(10.0,Cube[pcount]);
                pcount++;
        }

	double DMEQUAD = 0;
	if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
   		DMEQUAD=pow(10.0,Cube[pcount]);
		pcount++;
    	}	
	
	double WidthJitter = 0;
	if(((MNStruct *)globalcontext)->incWidthJitter > 0){
   		WidthJitter=pow(10.0,Cube[pcount]);
		if(((MNStruct *)globalcontext)->EQUADPriorType ==1) {uniformpriorterm += log(WidthJitter);}
		pcount++;
    	}

	double *ProfileNoiseAmps = new double[((MNStruct *)globalcontext)->ProfileNoiseCoeff]();
	int ProfileNoiseCoeff = 0;
	if(((MNStruct *)globalcontext)->incProfileNoise == 1){

		ProfileNoiseCoeff = ((MNStruct *)globalcontext)->ProfileNoiseCoeff;
		double ProfileNoiseAmp = pow(10.0, Cube[pcount]);
		pcount++;
		double ProfileNoiseSpec = Cube[pcount];
		pcount++;
		for(int q = 0; q < ProfileNoiseCoeff; q++){
			double profnoise = ProfileNoiseAmp*ProfileNoiseAmp*pow(double(q+1.0)/((MNStruct *)globalcontext)->ProfileInfo[0][1], ProfileNoiseSpec);
			ProfileNoiseAmps[q] = profnoise;
		}
	}

	if(((MNStruct *)globalcontext)->incProfileNoise == 2){

		ProfileNoiseCoeff = ((MNStruct *)globalcontext)->ProfileNoiseCoeff;
		double ProfileNoiseAmp = pow(10.0, Cube[pcount]);
		for(int q = 0; q < ProfileNoiseCoeff; q++){
			double ProfileNoiseAmp = pow(10.0, Cube[pcount]);
			pcount++;
			double profnoise = ProfileNoiseAmp*ProfileNoiseAmp;
			ProfileNoiseAmps[q] = profnoise;
		}
	}

        if(((MNStruct *)globalcontext)->incProfileNoise == 3){

                ProfileNoiseCoeff = ((MNStruct *)globalcontext)->ProfileNoiseCoeff;
                for(int q = 0; q < ProfileNoiseCoeff; q++){
                        double ProfileNoiseAmp = pow(10.0, Cube[pcount]);
                        pcount++;
                        double profnoise = ProfileNoiseAmp;
                        ProfileNoiseAmps[q] = profnoise;
                }
        }

        if(((MNStruct *)globalcontext)->incProfileNoise == 4){

                ProfileNoiseCoeff = ((MNStruct *)globalcontext)->ProfileNoiseCoeff;
                for(int q = 0; q < ProfileNoiseCoeff; q++){
                        double ProfileNoiseAmp = 1;
                        double profnoise = ProfileNoiseAmp;
                        ProfileNoiseAmps[q] = profnoise;
                }
        }


	double *SignalVec;
	double FreqDet = 0;
	double JDet = 0;
	double FreqLike = 0;
	int startpos = 0;
	if(((MNStruct *)globalcontext)->incDM > 4){

		int FitDMCoeff = 2*((MNStruct *)globalcontext)->numFitDMCoeff;
		double *SignalCoeff = new double[FitDMCoeff];
		for(int i = 0; i < FitDMCoeff; i++){
			SignalCoeff[startpos + i] = Cube[pcount];
			//printf("coeffs %i %g \n", i, SignalCoeff[i]);
			pcount++;
		}
			
		

		double Tspan = ((MNStruct *)globalcontext)->Tspan;
		double f1yr = 1.0/3.16e7;


		if(((MNStruct *)globalcontext)->incDM==5){

			double DMamp=Cube[pcount];
			pcount++;
			double DMindex=Cube[pcount];
			pcount++;

			
			DMamp=pow(10.0, DMamp);
			if(((MNStruct *)globalcontext)->DMPriorType == 1) { uniformpriorterm +=log(DMamp); }



			for (int i=0; i< FitDMCoeff/2; i++){
				
				double freq = ((double)(i+1.0))/Tspan;
				
				double rho = (DMamp*DMamp)*pow(f1yr,(-3)) * pow(freq*365.25,(-DMindex))/(Tspan*24*60*60);
				SignalCoeff[i] = SignalCoeff[i]*sqrt(rho);
				SignalCoeff[i+FitDMCoeff/2] = SignalCoeff[i+FitDMCoeff/2]*sqrt(rho);  
				FreqDet += 2*log(rho);	
				JDet += 2*log(sqrt(rho));
				FreqLike += SignalCoeff[i]*SignalCoeff[i]/rho + SignalCoeff[i+FitDMCoeff/2]*SignalCoeff[i+FitDMCoeff/2]/rho;
			}
		}


		if(((MNStruct *)globalcontext)->incDM==6){




			for (int i=0; i< FitDMCoeff/2; i++){
			

				double DMAmp = pow(10.0, Cube[pcount]);	
				double freq = ((double)(i+1.0))/Tspan;
				
				if(((MNStruct *)globalcontext)->DMPriorType == 1) { uniformpriorterm +=log(DMAmp); }
				double rho = (DMAmp*DMAmp);
				SignalCoeff[i] = SignalCoeff[i]*sqrt(rho);
				SignalCoeff[i+FitDMCoeff/2] = SignalCoeff[i+FitDMCoeff/2]*sqrt(rho);  
				FreqDet += 2*log(rho);	
				JDet += 2*log(sqrt(rho));
				FreqLike += SignalCoeff[i]*SignalCoeff[i]/rho + SignalCoeff[i+FitDMCoeff/2]*SignalCoeff[i+FitDMCoeff/2]/rho;
				pcount++;
			}
		}

		double *FMatrix = new double[FitDMCoeff*((MNStruct *)globalcontext)->numProfileEpochs];
		for(int i=0;i< FitDMCoeff/2;i++){
			int DMt = 0;
			for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[DMt].bat;
	
				FMatrix[k + (i+startpos)*((MNStruct *)globalcontext)->numProfileEpochs]=cos(2*M_PI*(double(i+1)/Tspan)*time);
				FMatrix[k + (i+FitDMCoeff/2+startpos)*((MNStruct *)globalcontext)->numProfileEpochs] = sin(2*M_PI*(double(i+1)/Tspan)*time);
				DMt += ((MNStruct *)globalcontext)->numChanPerInt[k];

			}
		}

		SignalVec = new double[((MNStruct *)globalcontext)->numProfileEpochs];
		vector_dgemv(FMatrix,SignalCoeff,SignalVec,((MNStruct *)globalcontext)->numProfileEpochs, FitDMCoeff,'N');
		startpos=FitDMCoeff;
		delete[] FMatrix;	
		

    	}

	double sinAmp = 0;
	double cosAmp = 0;
	if(((MNStruct *)globalcontext)->yearlyDM == 1){
		if(((MNStruct *)globalcontext)->incDM == 0){ SignalVec = new double[((MNStruct *)globalcontext)->numProfileEpochs]();}
		sinAmp = Cube[pcount];
		pcount++;
		cosAmp = Cube[pcount];
		pcount++;
		int DMt = 0;
		for(int o=0;o<((MNStruct *)globalcontext)->numProfileEpochs; o++){
			double time=(double)((MNStruct *)globalcontext)->pulse->obsn[DMt].bat - ((MNStruct *)globalcontext)->pulse->param[param_dmepoch].val[0];
			SignalVec[o] += sinAmp*sin((2*M_PI/365.25)*time) + cosAmp*cos((2*M_PI/365.25)*time);
			DMt += ((MNStruct *)globalcontext)->numChanPerInt[o];
		}
	}
/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	      //printf("badcorr? %i %g %g \n", i,  (double)((MNStruct *)globalcontext)->pulse->obsn[i].batCorr, (double)((MNStruct *)globalcontext)->pulse->obsn[i].residual);
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;
		
	      }
	      
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr - phase - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
		//printf("loop %i %i %g %.15Lg %.15Lg \n", loop, i, Cube[0], ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr, ((MNStruct *)globalcontext)->pulse->obsn[i].residual);
	 }


	int maxshapecoeff = 0;
	int totshapecoeff = ((MNStruct *)globalcontext)->totshapecoeff; 

	int *numcoeff= new int[((MNStruct *)globalcontext)->numProfComponents];
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		numcoeff[i] =  ((MNStruct *)globalcontext)->numshapecoeff[i];
		if(debug == 1){printf("num coeff in comp %i: %i \n", i, numcoeff[i]);}
	}

	
        int *numProfileStocCoeff = ((MNStruct *)globalcontext)->numshapestoccoeff;
        int totalshapestoccoeff = ((MNStruct *)globalcontext)->totalshapestoccoeff;


	int *numEvoCoeff = ((MNStruct *)globalcontext)->numEvoCoeff;
	int totalEvoCoeff = ((MNStruct *)globalcontext)->totalEvoCoeff;

	int *numEvoFitCoeff = ((MNStruct *)globalcontext)->numEvoFitCoeff;
	int totalEvoFitCoeff = ((MNStruct *)globalcontext)->totalEvoFitCoeff;

	int *numProfileFitCoeff = ((MNStruct *)globalcontext)->numProfileFitCoeff;
	int totalProfileFitCoeff = ((MNStruct *)globalcontext)->totalProfileFitCoeff;



	int totalCoeffForMult = 0;
	int *NumCoeffForMult = new int[((MNStruct *)globalcontext)->numProfComponents];
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		NumCoeffForMult[i] = 0;
		if(numEvoCoeff[i] > NumCoeffForMult[i]){NumCoeffForMult[i]=numEvoCoeff[i];}
		if(numProfileFitCoeff[i] > NumCoeffForMult[i]){NumCoeffForMult[i]=numProfileFitCoeff[i];}
		totalCoeffForMult += NumCoeffForMult[i];
		if(debug == 1){printf("num coeff for mult from comp %i: %i \n", i, NumCoeffForMult[i]);}
	}

        int *numShapeToSave = new int[((MNStruct *)globalcontext)->numProfComponents];
        for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
                numShapeToSave[i] = numProfileStocCoeff[i];
                if(numEvoCoeff[i] >  numShapeToSave[i]){numShapeToSave[i] = numEvoCoeff[i];}
                if(numProfileFitCoeff[i] >  numShapeToSave[i]){numShapeToSave[i] = numProfileFitCoeff[i];}
                if(debug == 1){printf("saved %i %i \n", i, numShapeToSave[i]);}
        }
        int totShapeToSave = 0;
        for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
                totShapeToSave += numShapeToSave[i];
        }

	double shapecoeff[totshapecoeff];
	double StocProfCoeffs[totalshapestoccoeff];
	double EvoCoeffs[totalEvoCoeff];
	double ProfileFitCoeffs[totalProfileFitCoeff];
	double ProfileModCoeffs[totalCoeffForMult];

	for(int i =0; i < totshapecoeff; i++){
		shapecoeff[i]=((MNStruct *)globalcontext)->MeanProfileShape[i];
	}
	for(int i =0; i < totalEvoCoeff; i++){
		EvoCoeffs[i]=((MNStruct *)globalcontext)->MeanProfileEvo[i];
		//printf("loaded evo coeff %i %g \n", i, EvoCoeffs[i]);
	}
	if(debug == 1){printf("Filled %i Coeff, %i EvoCoeff \n", totshapecoeff, totalEvoCoeff);}
	double beta = ((MNStruct *)globalcontext)->MeanProfileBeta*((MNStruct *)globalcontext)->ReferencePeriod;

	int cpos = 0;
	int epos = 0;
	int fpos = 0;
	for(int i =0; i < totalshapestoccoeff; i++){
		StocProfCoeffs[i]= pow(10.0, 2*Cube[pcount]);
		pcount++;
	}
	//printf("TotalProfileFit: %i \n",totalProfileFitCoeff );
	for(int i =0; i < totalProfileFitCoeff; i++){
	//	printf("ProfileFit: %i %i %g \n", i, pcount, Cube[pcount]);
		ProfileFitCoeffs[i]= Cube[pcount];
		pcount++;
	}
	double LinearProfileWidth=0;
	if(((MNStruct *)globalcontext)->FitLinearProfileWidth == 1){
		LinearProfileWidth = Cube[pcount];
		pcount++;
	}


	int  EvoFreqExponent = 1;
	if(((MNStruct *)globalcontext)->FitEvoExponent == 1){
		EvoFreqExponent =  floor(Cube[pcount]);
		//printf("Evo Exp set: %i %i \n", pcount, EvoFreqExponent);
		pcount++;
	}
	
	
	for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
		for(int i =0; i < numEvoFitCoeff[c]; i++){
			EvoCoeffs[i+cpos] += Cube[pcount];
			pcount++;
		}
		cpos += numEvoCoeff[c];
		
	}

	double EvoProfileWidth=0;
	if(((MNStruct *)globalcontext)->incProfileEvo == 2){
		EvoProfileWidth = Cube[pcount];
		pcount++;
	}

	double EvoEnergyProfileWidth=0;
	if(((MNStruct *)globalcontext)->incProfileEnergyEvo == 2){
		EvoEnergyProfileWidth = Cube[pcount];
		pcount++;
	}

	if(totshapecoeff+1>=totalshapestoccoeff+1){
		maxshapecoeff=totshapecoeff+1;
	}
	if(totalshapestoccoeff+1 > totshapecoeff+1){
		maxshapecoeff=totalshapestoccoeff+1;
	}

//	printf("max shape coeff: %i \n", maxshapecoeff);
/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	double modelflux=0;
	cpos = 0;
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		for(int j =0; j < numcoeff[i]; j=j+2){
			modelflux+=sqrt(sqrt(M_PI))*sqrt(beta)*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Binomial[j])*shapecoeff[cpos+j];
		}
		cpos+= numcoeff[i];
	}
	if(debug == 1){printf("model flux: %g \n", modelflux);}
	double ModModelFlux = modelflux;
	double OPLevel = ((MNStruct *)globalcontext)->offPulseLevel;


	double lnew = 0;
	int badshape = 0;

	int GlobalNBins = (int)((MNStruct *)globalcontext)->ProfileInfo[0][1];


	double **AllHermiteBasis =  new double*[GlobalNBins];
	double **JitterBasis  =  new double*[GlobalNBins];
	double **Hermitepoly =  new double*[GlobalNBins];

	for(int i =0;i<GlobalNBins;i++){AllHermiteBasis[i]=new double[maxshapecoeff];}
	for(int i =0;i<GlobalNBins;i++){Hermitepoly[i]=new double[totshapecoeff];}
	for(int i =0;i<GlobalNBins;i++){JitterBasis[i]=new double[totshapecoeff];}

	int ProfileBaselineTerms = ((MNStruct *)globalcontext)->ProfileBaselineTerms;
	int PerEpochSize = ProfileBaselineTerms + 1 + 2*ProfileNoiseCoeff;
	int Msize = 1+ProfileBaselineTerms+2*ProfileNoiseCoeff + totalshapestoccoeff;
	int Mcounter=0;
	if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
		Msize++;
	}
	if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
		Msize++;
	}
	if(((MNStruct *)globalcontext)->incWidthJitter > 0){
		Msize++;
	}

	double *M = new double[GlobalNBins*Msize];
	double *NM = new double[GlobalNBins*Msize];




//	for(int i =0; i < GlobalNBins; i++){
//		Mcounter=0;
//		M[i] = new double[Msize];
//		NM[i] = new double[Msize];
//
//	}


	double *MNM = new double[Msize*Msize];
//	for(int i =0; i < Msize; i++){
//	    MNM[i] = new double[Msize];
//	}



	if(dotime == 1){

		gettimeofday(&tval_after, NULL);
		timersub(&tval_after, &tval_before, &tval_resultone);
		printf("Time elapsed Up to Start of main loop: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
		//gettimeofday(&tval_before, NULL);
		
	}	
	//for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){


	int minep = 0;
	int maxep = ((MNStruct *)globalcontext)->numProfileEpochs;
	int t = 0;
	if(((MNStruct *)globalcontext)->SubIntToFit != -1){
		minep = ((MNStruct *)globalcontext)->SubIntToFit;
		maxep = minep+1;
		for(int ep = 0; ep < minep; ep++){	
			t += ((MNStruct *)globalcontext)->numChanPerInt[ep];
		}
	}
	//printf("eps: %i %i \n", minep, maxep);	
	for(int ep = minep; ep < maxep; ep++){
	
		double *EpochMNM;
		double *EpochdNM;
		double *EpochTempdNM;
		int EpochMsize = 0;

		int NChanInEpoch = ((MNStruct *)globalcontext)->numChanPerInt[ep];
		int NEpochBins = NChanInEpoch*GlobalNBins;

		if(((MNStruct *)globalcontext)->incWideBandNoise == 1){

			EpochMsize = PerEpochSize*NChanInEpoch+totalshapestoccoeff;
			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
				EpochMsize++;
			}
			if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
				EpochMsize++;
			}
			if(((MNStruct *)globalcontext)->incWidthJitter > 0){
				EpochMsize++;
			}

			EpochMNM = new double[EpochMsize*EpochMsize]();
			//for(int i =0; i < EpochMsize; i++){
			//    EpochMNM[i] = new double[EpochMsize]();
			//}

			EpochdNM = new double[EpochMsize]();
			EpochTempdNM = new double[EpochMsize]();
		}

		double EpochChisq = 0;	
		double EpochdetN = 0;
		double EpochLike = 0;



		for(int ch = 0; ch < NChanInEpoch; ch++){

			if(dotime == 1){	
				gettimeofday(&tval_before, NULL);
			}
			if(debug == 1){
				printf("In toa %i \n", t);
				printf("sat: %.15Lg \n", ((MNStruct *)globalcontext)->pulse->obsn[t].sat);
			}
			int nTOA = t;

			double detN  = 0;
			double Chisq  = 0;
			double logMargindet = 0;
			double Marginlike = 0;	 
			double JitterPrior = 0;	   

			double profilelike=0;

			long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
			long double FoldingPeriodDays = FoldingPeriod/SECDAY;
			int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
			double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
			double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
			long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

			double *Betatimes = new double[Nbins];
	

			double *shapevec  = new double[Nbins];
			double *Jittervec = new double[Nbins];
			double *ProfileModVec = new double[Nbins]();
			double *ProfileJitterModVec = new double[Nbins]();
		

			double noisemean=0;
			double MLSigma = 0;
			double dataflux = 0;


			double maxamp = 0;
			double maxshape=0;

			//if(ep==36){printf("SSB %i %.10g %.10g \n", nTOA, (double)((MNStruct *)globalcontext)->pulse->obsn[t].freqSSB/pow(10.0, 6), (double)((MNStruct *)globalcontext)->pulse->obsn[t].freq);}

	//
	//		if(debug == 1)printf("noise %g, dataflux %g, modelflux %g, maxamp %g \n", MLSigma, dataflux, modelflux, maxamp);


		    
			long double binpos = ModelBats[nTOA]; 
			//binpos += (long double)(((MNStruct *)globalcontext)->StoredDMVals[ep]/SECDAY);
			if(((MNStruct *)globalcontext)->incDM > 4 || ((MNStruct *)globalcontext)->yearlyDM == 1){
	                	double DMKappa = 2.410*pow(10.0,-16);
        		        double DMScale = 1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[t].freqSSB,2));
				//printf("DMSig %i %g\n", t, SignalVec[ep]*DMScale);
				long double DMshift = (long double)(SignalVec[ep]*DMScale);

				//DMshift = (long double)(((((MNStruct *)globalcontext)->StoredDMVals[ep]-((MNStruct *)globalcontext)->pulse->param[param_dm].val[0])*DMScale));

				//printf("DMSig: %i %.10g %g %Lg \n", nTOA, (double)((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat, double((((MNStruct *)globalcontext)->StoredDMVals[ep]-((MNStruct *)globalcontext)->pulse->param[param_dm].val[0])), DMshift);
				
		 		binpos+=DMshift/SECDAY;
			//	printf("After %.15Lg \n", binpos);

			}


			//if(binpos < ProfileBats[nTOA][0]){printf("UnderBoard! %.10Lg %.10Lg %.10Lg\n", binpos, ProfileBats[nTOA][0], ProfileBats[nTOA][Nbins-1]);}
			if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

			if(binpos > ProfileBats[nTOA][Nbins-1])binpos-=FoldingPeriodDays;


			if(binpos > ProfileBats[nTOA][Nbins-1]){printf("OverBoard! %.10Lg %.10Lg %.10Lg\n", binpos, ProfileBats[nTOA][Nbins-1], (binpos-ProfileBats[nTOA][Nbins-1])/FoldingPeriodDays);}

			long double minpos = binpos - FoldingPeriodDays/2;
			if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
			long double maxpos = binpos + FoldingPeriodDays/2;
			if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];



			int InterpBin = 0;
			double FirstInterpTimeBin = 0;
			int  NumWholeBinInterpOffset = 0;

			if(((MNStruct *)globalcontext)->InterpolateProfile == 1){

		
				long double timediff = 0;
				long double bintime = ProfileBats[t][0];


				if(bintime  >= minpos && bintime <= maxpos){
				    timediff = bintime - binpos;
				}
				else if(bintime < minpos){
				    timediff = FoldingPeriodDays+bintime - binpos;
				}
				else if(bintime > maxpos){
				    timediff = bintime - FoldingPeriodDays - binpos;
				}

				timediff=timediff*SECDAY;

				double OneBin = FoldingPeriod/Nbins;
				int NumBinsInTimeDiff = floor(timediff/OneBin + 0.5);
				double WholeBinsInTimeDiff = NumBinsInTimeDiff*FoldingPeriod/Nbins;
				double OneBinTimeDiff = -1*((double)timediff - WholeBinsInTimeDiff);

				double PWrappedTimeDiff = (OneBinTimeDiff - floor(OneBinTimeDiff/OneBin)*OneBin);

				if(debug == 1)printf("Making InterpBin: %g %g %i %g %g %g\n", (double)timediff, OneBin, NumBinsInTimeDiff, WholeBinsInTimeDiff, OneBinTimeDiff, PWrappedTimeDiff);

				InterpBin = floor(PWrappedTimeDiff/((MNStruct *)globalcontext)->InterpolatedTime+0.5);
				if(InterpBin >= ((MNStruct *)globalcontext)->NumToInterpolate)InterpBin -= ((MNStruct *)globalcontext)->NumToInterpolate;

				FirstInterpTimeBin = -1*(InterpBin-1)*((MNStruct *)globalcontext)->InterpolatedTime;

				if(debug == 1)printf("Interp Time Diffs: %g %g %g %g \n", ((MNStruct *)globalcontext)->InterpolatedTime, InterpBin*((MNStruct *)globalcontext)->InterpolatedTime, PWrappedTimeDiff, InterpBin*((MNStruct *)globalcontext)->InterpolatedTime-PWrappedTimeDiff);

				double FirstBinOffset = timediff-FirstInterpTimeBin;
				double dNumWholeBinOffset = FirstBinOffset/(FoldingPeriod/Nbins);
				int  NumWholeBinOffset = 0;

				NumWholeBinInterpOffset = floor(dNumWholeBinOffset+0.5);
	
				if(debug == 1)printf("Interp bin is: %i , First Bin is %g, Offset is %i \n", InterpBin, FirstInterpTimeBin, NumWholeBinInterpOffset);


			}
		   
			if(((MNStruct *)globalcontext)->InterpolateProfile == 0){

				for(int j =0; j < Nbins; j++){


					long double timediff = 0;
					long double bintime = ProfileBats[t][j];
					

					if(bintime  >= minpos && bintime <= maxpos){
					    timediff = bintime - binpos;
					}
					else if(bintime < minpos){
					    timediff = FoldingPeriodDays+bintime - binpos;
					}
					else if(bintime > maxpos){
					    timediff = bintime - FoldingPeriodDays - binpos;
					}


					timediff=timediff*SECDAY;


					Betatimes[j]=timediff/beta;
					TNothpl(maxshapecoeff,Betatimes[j],AllHermiteBasis[j]);

					for(int k =0; k < maxshapecoeff; k++){
						double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));
						AllHermiteBasis[j][k]=AllHermiteBasis[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);

						if(k<totshapecoeff){ Hermitepoly[j][k] = AllHermiteBasis[j][k];}

					}

					JitterBasis[j][0] = (1.0/sqrt(2.0))*(-1.0*Hermitepoly[j][1]);
					for(int k =1; k < totshapecoeff; k++){
						JitterBasis[j][k] = (1.0/sqrt(2.0))*(sqrt(double(k))*AllHermiteBasis[j][k-1] - sqrt(double(k+1))*AllHermiteBasis[j][k+1]);
					}

				}

				dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,totshapecoeff,'N');
				dgemv(JitterBasis,shapecoeff,Jittervec,Nbins,totshapecoeff,'N');


			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//////////////////////////////////////////////////////Get Noise Level//////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



				maxshape=0;
				for(int j =0; j < Nbins; j++){
					if(shapevec[j] > maxshape){ maxshape = shapevec[j];}
				}
				noisemean=0;
				int numoffpulse = 0;
				for(int j = 0; j < Nbins; j++){
					if(debug == 1 && nTOA == 0){printf("pulse: %i %g %g %g %g \n", j, shapevec[j],  maxshape,OPLevel,  maxshape*OPLevel);}
					if(shapevec[j] < maxshape*OPLevel){
						noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
						numoffpulse += 1;
					}
						
				}

				if(debug == 1)printf("noise mean %g num off pulse %i\n", noisemean, numoffpulse);
				noisemean = noisemean/(numoffpulse);

				MLSigma = 0;
				int MLSigmaCount = 0;
				dataflux = 0;
				for(int j = 0; j < Nbins; j++){
					if(shapevec[j] < maxshape*OPLevel){
						double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean;
						MLSigma += res*res; MLSigmaCount += 1;
					}
					else{
						if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean)/MLSigma > 1){
							dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-noisemean)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
						}
					}
				}

				MLSigma = sqrt(MLSigma/MLSigmaCount);
				MLSigma = MLSigma*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];
				maxamp = dataflux/modelflux;
				if(debug == 1)printf("noise %g, dataflux %g, modelflux %g, maxamp %g \n", MLSigma, dataflux, modelflux, maxamp);

		                for(int j =0; j < Nbins; j++){

				
					M[j] = 1;
					M[j + Nbins] = shapevec[j];

					Mcounter = 2;

					if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
						M[j + Mcounter*Nbins] = Jittervec[j]*dataflux/modelflux/beta;
						Mcounter++;
					}

					if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
						double DMKappa = 2.410*pow(10.0,-16);
						double DMScale = 1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[t].freqSSB,2));
						M[j + Mcounter*Nbins] = Jittervec[j]*DMScale*dataflux/modelflux/beta;
						Mcounter++;
					}

					int stocpos = 0;
					int savedpos=0;
					for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
						for(int k = 0; k < numProfileStocCoeff[c]; k++){
						    M[j + (Mcounter+k+stocpos)*Nbins] = AllHermiteBasis[j][k+savedpos]*dataflux;
						}
						stocpos+=numProfileStocCoeff[c];
						savedpos+=numShapeToSave[c];
					}
				}
			}
	
			if(dotime == 1){
				gettimeofday(&tval_after, NULL);
				timersub(&tval_after, &tval_before, &tval_resultone);
				printf("Time elapsed up to start of Interp: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
				gettimeofday(&tval_before, NULL);
			}

			double reffreq = ((MNStruct *)globalcontext)->EvoRefFreq;
			double freqdiff =  (((MNStruct *)globalcontext)->pulse->obsn[t].freq - reffreq)/1000.0;
			double freqscale = pow(freqdiff, EvoFreqExponent);


			double snr = ((MNStruct *)globalcontext)->pulse->obsn[t].snr;
			double tobs = ((MNStruct *)globalcontext)->pulse->obsn[t].tobs; 

			//printf("snr: %i %g %g %g \n", t, snr, tobs, snr*3600/tobs);
			snr = snr*3600/tobs;

			double refSN = 1000;
			double SNdiff =  snr/refSN;
			double SNscale = snr-refSN;
			//double SNscale = log10(snr/refSN);//pow(freqdiff, EvoFreqExponent);

			//printf("snr: %i %g %g %g \n", t, snr, SNdiff, SNscale);
			//printf("Evo Exp %i %i %g %g \n", t, EvoFreqExponent, freqdiff, freqscale);

			if(((MNStruct *)globalcontext)->InterpolateProfile == 1){

			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//////////////////////////////////////////////////////Add in any Profile Changes///////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				for(int i =0; i < totalCoeffForMult; i++){
					ProfileModCoeffs[i]=0;	
				}				

				cpos = 0;
				epos = 0;
				fpos = 0;
				for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
					
					for(int i =0; i < numProfileFitCoeff[c]; i++){
						ProfileModCoeffs[i+cpos] += ProfileFitCoeffs[i+fpos];
						//if(t==0){printf("Mod FitCoeff: %i %i %g \n", i, fpos,  ProfileFitCoeffs[i+fpos]);}
					}

					for(int i =0; i < numEvoCoeff[c]; i++){
						ProfileModCoeffs[i+cpos] += EvoCoeffs[i+epos]*freqscale;
						//if(t==0){printf("Mod EvoCoeff: %i %i %g \n", i, epos,   EvoCoeffs[i+epos]);}
					}

					cpos += NumCoeffForMult[c];
					epos += numEvoCoeff[c];
					fpos += numProfileFitCoeff[c];	
				}


				

				if(totalCoeffForMult > 0){
					vector_dgemv(((MNStruct *)globalcontext)->InterpolatedShapeletsVec[InterpBin], ProfileModCoeffs,ProfileModVec,Nbins,totalCoeffForMult,'N');

					if(((MNStruct *)globalcontext)->numFitEQUAD > 0 || ((MNStruct *)globalcontext)->incDMEQUAD > 0 ){
						vector_dgemv(((MNStruct *)globalcontext)->InterpolatedJitterProfileVec[InterpBin], ProfileModCoeffs,ProfileJitterModVec,Nbins,totalCoeffForMult,'N');
					}
				}
				ModModelFlux = modelflux;

				cpos=0;
				for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
					for(int j =0; j < NumCoeffForMult[c]; j=j+2){
						ModModelFlux+=sqrt(sqrt(M_PI))*sqrt(beta)*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Binomial[j])*ProfileModCoeffs[cpos+j];
					}
					cpos+= NumCoeffForMult[c];
				}

				if(dotime == 1){
					gettimeofday(&tval_after, NULL);
					timersub(&tval_after, &tval_before, &tval_resultone);
					printf("Time elapsed,  Modded Profile: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
					gettimeofday(&tval_before, NULL);
				}

			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//////////////////////////////////////////////////////Fill Arrays with interpolated state//////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				noisemean=0;
				int numoffpulse = 0;
		                maxshape=0;
				
				for(int j =0; j < Nbins; j++){

					double NewIndex = (j + NumWholeBinInterpOffset);
					int Nj =  Wrap(j + NumWholeBinInterpOffset, 0, Nbins-1);//(int)(NewIndex - floor(NewIndex/Nbins)*Nbins);

					double widthTerm = ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpBin][Nj]*LinearProfileWidth; 
					double evoWidthTerm = ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpBin][Nj]*EvoProfileWidth*freqscale;
					double SNWidthTerm = ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpBin][Nj]*EvoEnergyProfileWidth*SNscale;

					shapevec[j] = ((MNStruct *)globalcontext)->InterpolatedMeanProfile[InterpBin][Nj] + widthTerm + ProfileModVec[Nj] + evoWidthTerm + SNWidthTerm;
					//printf("widthterm %i %i %g %g %g\n", t, j, SNWidthTerm, EvoEnergyProfileWidth, SNscale);
					//if(t==0){printf("fill evo coeff %i %g %g %g %g %g\n", j, SNWidthTerm, ProfileModVec[Nj], widthTerm, evoWidthTerm,((MNStruct *)globalcontext)->InterpolatedMeanProfile[InterpBin][Nj]);}

					Mcounter = 0;
					for(int q = 0; q < ProfileBaselineTerms; q++){
						M[j+Mcounter*Nbins] = pow(double(j)/Nbins, q);
						Mcounter++;
						//printf("Baseline: %i %i %i %g \n", t, j, q, M[0]);
					}
					
					M[j + Mcounter*Nbins] = shapevec[j];
					if(shapevec[j] > maxshape){ maxshape = shapevec[j];}
					Mcounter++;


					for(int q = 0; q < ProfileNoiseCoeff; q++){

						M[j+Mcounter*Nbins] =     cos(2*M_PI*(double(q+1)/Nbins)*j);
						M[j+(Mcounter+1)*Nbins] = sin(2*M_PI*(double(q+1)/Nbins)*j);

						Mcounter += 2;
					}

				
					if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
						M[j + Mcounter*Nbins] = ((MNStruct *)globalcontext)->InterpolatedJitterProfile[InterpBin][Nj] + ProfileJitterModVec[Nj];

						Mcounter++;
					}		

					if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
						double DMKappa = 2.410*pow(10.0,-16);
						double DMScale = 1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[t].freqSSB,2));
						M[j + Mcounter*Nbins] = (((MNStruct *)globalcontext)->InterpolatedJitterProfile[InterpBin][Nj] + ProfileJitterModVec[Nj])*DMScale;
						Mcounter++;
					}	


					if(((MNStruct *)globalcontext)->incWidthJitter > 0){
						M[j + Mcounter*Nbins] = ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpBin][Nj];
						Mcounter++;
					}

					int stocpos=0;
					int savepos=0;
					for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
						for(int k = 0; k < numProfileStocCoeff[c]; k++){
						    M[j + (Mcounter+k+stocpos)*Nbins] = ((MNStruct *)globalcontext)->InterpolatedShapeletsVec[InterpBin][Nj + (k+savepos)*Nbins];
						}
						stocpos+=numProfileStocCoeff[c];
						savepos+=numShapeToSave[c];

					}
					//printf("toa: %i %i %g %g %g %g\n", nTOA, j, ((MNStruct *)globalcontext)->InterpolatedMeanProfile[InterpBin][Nj] , widthTerm , ProfileModVec[Nj], ((MNStruct *)globalcontext)->MaxShapeAmp*OPLevel);
						
				}


				for(int j =0; j < Nbins; j++){
					//if(debug == 1 && nTOA == 0){printf("pulse: %i %g %g %g %g \n", j, shapevec[j],  maxshape,OPLevel,  maxshape*OPLevel);}
					if(shapevec[j] < maxshape*OPLevel){
						noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
						numoffpulse += 1;
						//if(nTOA==4)printf("off pulse: %i %g %g\n", j, (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1], shapevec[j]);
					}	
				}

				if(debug == 1)printf("noise mean %g num off pulse %i\n", noisemean, numoffpulse);

				if(dotime == 1){
					gettimeofday(&tval_after, NULL);
					timersub(&tval_after, &tval_before, &tval_resultone);
					printf("Time elapsed,  Filled Arrays: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
					gettimeofday(&tval_before, NULL);
				}

			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//////////////////////////////////////////////////////Get Noise Level//////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				noisemean = noisemean/(numoffpulse);

				MLSigma = 0;
				int MLSigmaCount = 0;
				dataflux = 0;
				for(int j = 0; j < Nbins; j++){
					if(shapevec[j] < maxshape*OPLevel){
						double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean;
						MLSigma += res*res; MLSigmaCount += 1;
					}
				}

				MLSigma = sqrt(MLSigma/MLSigmaCount);
				MLSigma = MLSigma*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]]*((MNStruct *)globalcontext)->MLProfileNoise[ep][0];

				dataflux = 0;
				for(int j = 0; j < Nbins; j++){
					if(shapevec[j] >= maxshape*OPLevel){
						if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean)/MLSigma > 1){
							dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-noisemean)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
						}
					}
				}

				maxamp = dataflux/ModModelFlux;

				if(debug == 1)printf("noise %g, dataflux %g, modelflux %g, maxamp %g \n", MLSigma, dataflux, ModModelFlux, maxamp);


				if(dotime == 1){
					gettimeofday(&tval_after, NULL);
					timersub(&tval_after, &tval_before, &tval_resultone);
					printf("Time elapsed,  Get Noise: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
					gettimeofday(&tval_before, NULL);
				}

			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//////////////////////////////////////////////////////Rescale Basis Vectors////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


				Mcounter = 1+ProfileBaselineTerms+ProfileNoiseCoeff*2;
				if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
					for(int j =0; j < Nbins; j++){
						M[j + Mcounter*Nbins] = M[j + Mcounter*Nbins]*dataflux/ModModelFlux/beta;
					}			
					Mcounter++;
				}
				if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
					for(int j =0; j < Nbins; j++){
						M[j + Mcounter*Nbins] = M[j + Mcounter*Nbins]*dataflux/ModModelFlux/beta;
					}			
					Mcounter++;
				}
				if(((MNStruct *)globalcontext)->incWidthJitter > 0){
					for(int j =0; j < Nbins; j++){
						M[j + Mcounter*Nbins] = M[j + Mcounter*Nbins]*dataflux/ModModelFlux;
					}
					Mcounter++;
				}
				for(int k = 0; k < totalshapestoccoeff; k++){
					for(int j =0; j < Nbins; j++){
				   		M[j + (Mcounter+k)*Nbins] = M[j + (Mcounter+k)*Nbins]*dataflux;
					}
				}


		    }

	
		    double *NDiffVec = new double[Nbins];

	//	    double maxshape=0;
	//	    for(int j =0; j < Nbins; j++){
	// 	        if(shapevec[j] > maxshape){ maxshape = shapevec[j];}
	//	    }

	
		///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


			      
			Chisq = 0;
			detN = 0;
			double OffPulsedetN = log(MLSigma*MLSigma);
			double OneDetN = 0;
			double logconvfactor = 1.0/log2(exp(1));

	
	 
			double highfreqnoise = HighFreqStoc1;
			double flatnoise = (HighFreqStoc2*maxamp*maxshape)*(HighFreqStoc2*maxamp*maxshape);
			

			for(int i =0; i < Nbins; i++){
			  	Mcounter=2;
				double noise = MLSigma*MLSigma;

				OneDetN=OffPulsedetN;
				if(shapevec[i] > maxshape*OPLevel && ((MNStruct *)globalcontext)->incHighFreqStoc > 0){
					noise +=  flatnoise + (highfreqnoise*maxamp*shapevec[i])*(highfreqnoise*maxamp*shapevec[i]);
					OneDetN=log2(noise)*logconvfactor;
				}

				detN += OneDetN;
				noise=1.0/noise;
				
				double datadiff =  ((MNStruct *)globalcontext)->ProfileData[nTOA][i][1];
				//printf("Data: %g %i %g %g %g\n", Cube[0], i, datadiff, 1.0/sqrt(noise), shapevec[i]);
				NDiffVec[i] = datadiff*noise;

				Chisq += datadiff*NDiffVec[i];
	

				
				for(int j = 0; j < Msize; j++){
					NM[i + j*Nbins] = M[i + j*Nbins]*noise;
				}

				

			}

			if(dotime == 1){
				gettimeofday(&tval_after, NULL);
				timersub(&tval_after, &tval_before, &tval_resultone);
				printf("Time elapsed,  Up to LA: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
				gettimeofday(&tval_before, NULL);
			}

			vector_dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');


			double *dNM = new double[Msize];
			double *TempdNM = new double[Msize];
			vector_dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		
			if(((MNStruct *)globalcontext)->incWideBandNoise == 1){

				int ppterms = PerEpochSize;
				if(debug==1){printf("Filling EpochMNM: %i %i %i \n", ppterms, Msize, EpochMsize);}
				for(int j = 0; j < ppterms; j++){
					EpochdNM[ppterms*ch+j] = dNM[j];
					for(int k = 0; k < ppterms; k++){
						if(debug==1){if(j==k && ep == 0){printf("Filling EpochMNM diag: %i %i %i %g \n", j,k,ppterms*ch+j + (ppterms*ch+k)*EpochMsize, MNM[j + k*Msize]);}}
						EpochMNM[ppterms*ch+j + (ppterms*ch+k)*EpochMsize] = MNM[j + k*Msize];
					}
				}


				for(int j = 0; j < Msize-ppterms; j++){

					EpochdNM[ppterms*NChanInEpoch+j] += dNM[ppterms+j];

				//	for(int k = 0; k < ppterms; k++){

						for(int q = 0; q < ppterms; q++){
							EpochMNM[ppterms*ch+q + (ppterms*NChanInEpoch+j)*EpochMsize] = MNM[q + (ppterms+j)*Msize];
							//EpochMNM[ppterms*ch+1 + (ppterms*NChanInEpoch+j)*EpochMsize] = MNM[1 + (ppterms+j)*Msize];

							EpochMNM[ppterms*NChanInEpoch+j + (ppterms*ch+q)*EpochMsize] = MNM[ppterms+j + q*Msize];
							//EpochMNM[ppterms*NChanInEpoch+j + (ppterms*ch+1)*EpochMsize] = MNM[ppterms+j + 1*Msize];
						}
				//	}

					for(int k = 0; k < Msize-ppterms; k++){

						EpochMNM[ppterms*NChanInEpoch+j + (ppterms*NChanInEpoch+k)*EpochMsize] += MNM[ppterms+j + (ppterms+k)*Msize];
					}
				}
			}


	

			double Margindet = 0;
			double StocProfDet = 0;
			double JitterDet = 0;
			if(((MNStruct *)globalcontext)->incWideBandNoise == 0){


				Mcounter=1+ProfileBaselineTerms;

		

				if(((MNStruct *)globalcontext)->incProfileNoise == 1 || ((MNStruct *)globalcontext)->incProfileNoise == 2){
					for(int q = 0; q < ProfileNoiseCoeff; q++){
						double profnoise = ProfileNoiseAmps[q];
						JitterDet += 2*log(profnoise);
						MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
						Mcounter++;
						MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
						Mcounter++;
					}
				}

				if(((MNStruct *)globalcontext)->incProfileNoise == 3 || ((MNStruct *)globalcontext)->incProfileNoise == 4){
					for(int q = 0; q < ProfileNoiseCoeff; q++){
						if(debug == 1){if(ep==0){printf("PN BW %i %i %i %i %g %g \n", ep, ch, Mcounter, q, MNM[Mcounter + Mcounter*Msize], MNM[Mcounter+1 + (Mcounter+1)*Msize]);}}
						double profnoise = ProfileNoiseAmps[q]*((MNStruct *)globalcontext)->MLProfileNoise[ep][q+1];
						JitterDet += 2*log(profnoise);
						MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
						Mcounter++;
						MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
						Mcounter++;
					}
				}

				if(((MNStruct *)globalcontext)->numFitEQUAD > 0){

					double profnoise = EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
					profnoise = profnoise*profnoise;
				    	JitterDet += log(profnoise);
				    	MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
					Mcounter++;
				}

				if(((MNStruct *)globalcontext)->incDMEQUAD > 0){

					double profnoise = DMEQUAD;
					profnoise = profnoise*profnoise;
				    	JitterDet += log(profnoise);
				    	MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
					Mcounter++;
				}


				if(((MNStruct *)globalcontext)->incWidthJitter > 0){

					double profnoise = WidthJitter;
					profnoise = profnoise*profnoise;
				    	JitterDet += log(profnoise);
				    	MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
					Mcounter++;
				}
				
				for(int j = 0; j < totalshapestoccoeff; j++){
					StocProfDet += log(StocProfCoeffs[j]);
					MNM[Mcounter+j + (Mcounter+j)*Msize] +=  1.0/StocProfCoeffs[j];
				}


				for(int i =0; i < Msize; i++){
					TempdNM[i] = dNM[i];
				}


				int info=0;
				
				vector_dpotrfInfo(MNM, Msize, Margindet, info);
				vector_dpotrsInfo(MNM, TempdNM, Msize, info);
					    
				logMargindet = Margindet;

				Marginlike = 0;
				for(int i =0; i < Msize; i++){
					Marginlike += TempdNM[i]*dNM[i];
				}
			}

			profilelike = -0.5*(detN + Chisq + logMargindet + StocProfDet + JitterDet - Marginlike);
			//lnew += profilelike;
			if(debug == 1)printf("Like: %i %.15g %.15g %.15g %.15g %.15g %.15g %.15g\n", nTOA,lnew, detN, Chisq, logMargindet, Marginlike, StocProfDet , JitterDet);

			EpochChisq+=Chisq;
			EpochdetN+=detN;
			EpochLike+=profilelike;

			delete[] shapevec;
			delete[] Jittervec;
			delete[] ProfileModVec;
			delete[] ProfileJitterModVec;
			delete[] NDiffVec;
			delete[] dNM;
			delete[] TempdNM;
			delete[] Betatimes;

			if(dotime == 1){
				gettimeofday(&tval_after, NULL);
				timersub(&tval_after, &tval_before, &tval_resultone);
				printf("Time elapsed,  End of Epoch: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
			}

			t++;
		}



///////////////////////////////////////////

		if(dotime == 1){
			gettimeofday(&tval_before, NULL);
		}

		if(((MNStruct *)globalcontext)->incWideBandNoise == 0){

			lnew += EpochLike;
		}

		if(((MNStruct *)globalcontext)->incWideBandNoise == 1){

			for(int i =0; i < EpochMsize; i++){
				EpochTempdNM[i] = EpochdNM[i];
			}

			int EpochMcount=0;
			if(((MNStruct *)globalcontext)->incProfileNoise == 0){EpochMcount = (1+ProfileBaselineTerms)*NChanInEpoch;}

			double BandJitterDet = 0;

			if(((MNStruct *)globalcontext)->incProfileNoise == 3 || ((MNStruct *)globalcontext)->incProfileNoise == 4){
				for(int ch = 0; ch < ((MNStruct *)globalcontext)->numChanPerInt[ep]; ch++){
					
					EpochMcount += 1+ProfileBaselineTerms;
					for(int q = 0; q < ProfileNoiseCoeff; q++){
						if(debug == 1){printf("PN BW %i %i %i %i %g %g \n", ep, ch, EpochMcount, q, EpochMNM[EpochMcount + EpochMcount*EpochMsize], EpochMNM[EpochMcount+1 + (EpochMcount+1)*EpochMsize]);}
						double profnoise = ((MNStruct *)globalcontext)->MLProfileNoise[ep][q+1];
						BandJitterDet += 2*log(profnoise);
						EpochMNM[EpochMcount + EpochMcount*EpochMsize] += 1.0/profnoise;
						EpochMcount++;
						EpochMNM[EpochMcount + EpochMcount*EpochMsize] += 1.0/profnoise;
						EpochMcount++;
					}
				}
			}

			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){

				double profnoise = EQUAD[((MNStruct *)globalcontext)->sysFlags[0]];
				profnoise = profnoise*profnoise;
			    	BandJitterDet += log(profnoise);

				EpochMNM[EpochMcount + (EpochMcount)*EpochMsize] += 1.0/profnoise;
				EpochMcount++;
			}

			if(((MNStruct *)globalcontext)->incDMEQUAD > 0){

				double profnoise = DMEQUAD;
				profnoise = profnoise*profnoise;
			    	BandJitterDet += log(profnoise);

				EpochMNM[EpochMcount + (EpochMcount)*EpochMsize] += 1.0/profnoise;
				EpochMcount++;
			}

			if(((MNStruct *)globalcontext)->incWidthJitter > 0){

				double profnoise = WidthJitter;
				profnoise = profnoise*profnoise;
			    	BandJitterDet += log(profnoise);

				EpochMNM[EpochMcount + (EpochMcount)*EpochMsize] += 1.0/profnoise;
				EpochMcount++;
			}

			double BandStocProfDet = 0;
			for(int j = 0; j < totalshapestoccoeff; j++){
				BandStocProfDet += log(StocProfCoeffs[j]);
				EpochMNM[EpochMcount+j + (EpochMcount+j)*EpochMsize] += 1.0/StocProfCoeffs[j];
			}

			int Epochinfo=0;
			double EpochMargindet = 0;
			vector_dpotrfInfo(EpochMNM, EpochMsize, EpochMargindet, Epochinfo);
			vector_dpotrsInfo(EpochMNM, EpochTempdNM, EpochMsize, Epochinfo);
				    
			double EpochMarginlike = 0;
			for(int i =0; i < EpochMsize; i++){
				EpochMarginlike += EpochTempdNM[i]*EpochdNM[i];
			}

			double EpochProfileLike = -0.5*(EpochdetN + EpochChisq + EpochMargindet - EpochMarginlike + BandJitterDet + BandStocProfDet);
			lnew += EpochProfileLike;
			if(debug == 1){printf("BWLike %i %g %g %g %g %g %g \n", ep,EpochdetN ,EpochChisq ,EpochMargindet , EpochMarginlike, BandJitterDet, BandStocProfDet );}



//			printf("Compare likes: %i %g %g %g %g %g %g\n", ep, EpochProfileLike, BandJitterDet, EpochMarginlike, EpochMargindet, EpochChisq, EpochdetN);
			//printf("like %i %g \n", t, EpochProfileLike);

			//for (int j = 0; j < EpochMsize; j++){
			//    delete[] EpochMNM[j];
			//}
			delete[] EpochMNM;
			delete[] EpochdNM;
			delete[] EpochTempdNM;

			if(dotime == 1){
				gettimeofday(&tval_after, NULL);
				timersub(&tval_after, &tval_before, &tval_resultone);
				printf("Time elapsed,  End of Band: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
			}
		}

////////////////////////////////////////////
	
	}
	 
	 
	for (int j = 0; j < GlobalNBins; j++){
	    delete[] Hermitepoly[j];
	    delete[] JitterBasis[j];
	    delete[] AllHermiteBasis[j];
	   // delete[] M[j];
	   // delete[] NM[j];
	}
	delete[] Hermitepoly;
	delete[] AllHermiteBasis;
	delete[] JitterBasis;
	delete[] M;
	delete[] NM;

	//for (int j = 0; j < Msize; j++){
	//    delete[] MNM[j];
	//}
	delete[] MNM;
	 

	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;
	delete[] numcoeff;
	delete[] NumCoeffForMult;
	delete[] ProfileNoiseAmps;
	if(((MNStruct *)globalcontext)->incDM > 0 || ((MNStruct *)globalcontext)->yearlyDM == 1){
		delete[] SignalVec;
	}
	lnew += uniformpriorterm - 0.5*(FreqLike + FreqDet) + JDet;
	//printf("Cube: %g %g %g %g %g \n", lnew+5.8185e6, FreqLike, FreqDet, Cube[19], Cube[20]);
	if(debug == 1)printf("End Like: %.10g \n", lnew);
	//printf("End Like: %.10g \n", lnew);
	//}


     //	gettimeofday(&tval_after, NULL);
       //	timersub(&tval_after, &tval_before, &tval_resulttwo);
      // 	printf("Time elapsed: %ld.%06ld %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec, (long int)tval_resulttwo.tv_sec, (long int)tval_resulttwo.tv_usec);
	//}
	//sleep(5);
	return lnew;
	
	//return bluff;
	//sleep(5);

}

void  WriteProfileDomainLike(std::string longname, int &ndim){



	int writeprofiles=1;

	int profiledimstart=0;
        double *Cube = new double[ndim];
        int number_of_lines = 0;

        std::ifstream checkfile;
	std::string checkname;
	if(((MNStruct *)globalcontext)->sampler == 0){
	        checkname = longname+"phys_live.points";
	}
        if(((MNStruct *)globalcontext)->sampler == 1){
                checkname = longname+"_phys_live.txt";
        }
	//printf("%i %s \n", ((MNStruct *)globalcontext)->sampler, checkname.c_str());
        checkfile.open(checkname.c_str());
        std::string line;
        while (getline(checkfile, line))
                ++number_of_lines;

        checkfile.close();

        std::ifstream summaryfile;
	std::string fname;
	if(((MNStruct *)globalcontext)->sampler == 0){
		fname = longname+"phys_live.points";
	}
	if(((MNStruct *)globalcontext)->sampler == 1){
	        fname = longname+"_phys_live.txt";
	}

        summaryfile.open(fname.c_str());



        printf("Getting ML \n");
        double maxlike = -1.0*pow(10.0,10);
        for(int i=0;i<number_of_lines;i++){

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);
                double like = paramlist[ndim];

                if(like > maxlike){
                        maxlike = like;
                         for(int i = 0; i < ndim; i++){
                                 Cube[i] = paramlist[i];
                         }
                }

        }
        summaryfile.close();
	printf("ML val: %.10g \n", maxlike);

	int dotime = 0;
	struct timeval tval_before, tval_after, tval_resultone, tval_resulttwo;


	//gettimeofday(&tval_before, NULL);
	//double bluff = 0;
	//for(int loop = 0; loop < 5; loop++){
	//	Cube[0] = loop*0.2;

	int debug = 0;

	if(debug == 1)printf("In like \n");

	double *EFAC;
	double *EQUAD;
        long double LDparams[ndim];
        int pcount;

	double uniformpriorterm = 0;



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){

		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

	}	
	
	
	pcount=0;
	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
           	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
                	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){
                        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
			}
                 }
           }

	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form SATS///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}

	///for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
	//	((MNStruct *)globalcontext)->pulse->jumpVal[k]= 0;
	//}
       for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
              ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }

	delete[] JumpVec;


	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);    
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);     
	



	
	if(debug == 1)printf("Phase: %g \n", (double)phase);
	if(debug == 1)printf("Formed Residuals \n");

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////White noise/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=1;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[0]);}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[p]);}
			pcount++;

		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
                }
        }


        double *SQUAD;
	if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
        	SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
        	for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
           		SQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
        	}
    	}	  
	  

	double HighFreqStoc1 = 0;
	double HighFreqStoc2 = 0;
	if(((MNStruct *)globalcontext)->incHighFreqStoc == 1){
		HighFreqStoc1 = pow(10.0,Cube[pcount]);
		pcount++;
	}
        if(((MNStruct *)globalcontext)->incHighFreqStoc == 2){
                HighFreqStoc1 = pow(10.0,Cube[pcount]);
                pcount++;
		HighFreqStoc2 = pow(10.0,Cube[pcount]);
                pcount++;
        }

	double DMEQUAD = 0;
	if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
   		DMEQUAD=pow(10.0,Cube[pcount]);
		pcount++;
    	}	
	
	double WidthJitter = 0;
	if(((MNStruct *)globalcontext)->incWidthJitter > 0){
   		WidthJitter=pow(10.0,Cube[pcount]);
		pcount++;
    	}



	double *ProfileNoiseAmps = new double[((MNStruct *)globalcontext)->ProfileNoiseCoeff]();
	int ProfileNoiseCoeff = 0;
	if(((MNStruct *)globalcontext)->incProfileNoise == 1){

		ProfileNoiseCoeff = ((MNStruct *)globalcontext)->ProfileNoiseCoeff;
		double ProfileNoiseAmp = pow(10.0, Cube[pcount]);
		pcount++;
		double ProfileNoiseSpec = Cube[pcount];
		pcount++;
		for(int q = 0; q < ProfileNoiseCoeff; q++){
			double profnoise = ProfileNoiseAmp*ProfileNoiseAmp*pow(double(q+1)/((MNStruct *)globalcontext)->ProfileInfo[0][1], ProfileNoiseSpec);
			ProfileNoiseAmps[q] = profnoise;
		}
	}

	if(((MNStruct *)globalcontext)->incProfileNoise == 2){

		ProfileNoiseCoeff = ((MNStruct *)globalcontext)->ProfileNoiseCoeff;
		for(int q = 0; q < ProfileNoiseCoeff; q++){
			double ProfileNoiseAmp = pow(10.0, Cube[pcount]);
			pcount++;
			double profnoise = ProfileNoiseAmp*ProfileNoiseAmp;
			ProfileNoiseAmps[q] = profnoise;
		}
	}

        if(((MNStruct *)globalcontext)->incProfileNoise == 3){

                ProfileNoiseCoeff = ((MNStruct *)globalcontext)->ProfileNoiseCoeff;
                for(int q = 0; q < ProfileNoiseCoeff; q++){
                        double ProfileNoiseAmp = pow(10.0, Cube[pcount]);
                        pcount++;
                        double profnoise = ProfileNoiseAmp;
                        ProfileNoiseAmps[q] = profnoise;
                }
        }

        if(((MNStruct *)globalcontext)->incProfileNoise == 4){

                ProfileNoiseCoeff = ((MNStruct *)globalcontext)->ProfileNoiseCoeff;
                for(int q = 0; q < ProfileNoiseCoeff; q++){
                        double ProfileNoiseAmp = 1;
                        double profnoise = ProfileNoiseAmp;
                        ProfileNoiseAmps[q] = profnoise;
                }
        }



	double *SignalVec;
	double FreqDet = 0;
	double JDet = 0;
	double FreqLike = 0;
	int startpos = 0;
	if(((MNStruct *)globalcontext)->incDM > 4){

		int FitDMCoeff = 2*((MNStruct *)globalcontext)->numFitDMCoeff;
		double *SignalCoeff = new double[FitDMCoeff];
		for(int i = 0; i < FitDMCoeff; i++){
			SignalCoeff[startpos + i] = Cube[pcount];
			//printf("coeffs %i %g \n", i, SignalCoeff[i]);
			pcount++;
		}
			
		

		double Tspan = ((MNStruct *)globalcontext)->Tspan;
		double f1yr = 1.0/3.16e7;


		if(((MNStruct *)globalcontext)->incDM==5){

			double DMamp=Cube[pcount];
			pcount++;
			double DMindex=Cube[pcount];
			pcount++;

			
			DMamp=pow(10.0, DMamp);
			if(((MNStruct *)globalcontext)->DMPriorType == 1) { uniformpriorterm +=log(DMamp); }



			for (int i=0; i< FitDMCoeff/2; i++){
				
				double freq = ((double)(i+1.0))/Tspan;
				
				double rho = (DMamp*DMamp)*pow(f1yr,(-3)) * pow(freq*365.25,(-DMindex))/(Tspan*24*60*60);
				SignalCoeff[i] = SignalCoeff[i]*sqrt(rho);
				SignalCoeff[i+FitDMCoeff/2] = SignalCoeff[i+FitDMCoeff/2]*sqrt(rho);  
				FreqDet += 2*log(rho);	
				JDet += 2*log(sqrt(rho));
				FreqLike += SignalCoeff[i]*SignalCoeff[i]/rho + SignalCoeff[i+FitDMCoeff/2]*SignalCoeff[i+FitDMCoeff/2]/rho;
			}
		}


		if(((MNStruct *)globalcontext)->incDM==6){




			for (int i=0; i< FitDMCoeff/2; i++){
			

				double DMAmp = pow(10.0, Cube[pcount]);	
				double freq = ((double)(i+1.0))/Tspan;
				
				if(((MNStruct *)globalcontext)->DMPriorType == 1) { uniformpriorterm +=log(DMAmp); }
				double rho = (DMAmp*DMAmp);
				SignalCoeff[i] = SignalCoeff[i]*sqrt(rho);
				SignalCoeff[i+FitDMCoeff/2] = SignalCoeff[i+FitDMCoeff/2]*sqrt(rho);  
				FreqDet += 2*log(rho);	
				JDet += 2*log(sqrt(rho));
				FreqLike += SignalCoeff[i]*SignalCoeff[i]/rho + SignalCoeff[i+FitDMCoeff/2]*SignalCoeff[i+FitDMCoeff/2]/rho;
				pcount++;
			}
		}

		double *FMatrix = new double[FitDMCoeff*((MNStruct *)globalcontext)->numProfileEpochs];
		for(int i=0;i< FitDMCoeff/2;i++){
			int DMt = 0;
			for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[DMt].bat;
	
				FMatrix[k + (i+startpos)*((MNStruct *)globalcontext)->numProfileEpochs]=cos(2*M_PI*(double(i+1)/Tspan)*time);
				FMatrix[k + (i+FitDMCoeff/2+startpos)*((MNStruct *)globalcontext)->numProfileEpochs] = sin(2*M_PI*(double(i+1)/Tspan)*time);
				DMt += ((MNStruct *)globalcontext)->numChanPerInt[k];

			}
		}

		SignalVec = new double[((MNStruct *)globalcontext)->numProfileEpochs];
		vector_dgemv(FMatrix,SignalCoeff,SignalVec,((MNStruct *)globalcontext)->numProfileEpochs, FitDMCoeff,'N');
		startpos=FitDMCoeff;
		delete[] FMatrix;	
		

    	}

	double sinAmp = 0;
	double cosAmp = 0;
	if(((MNStruct *)globalcontext)->yearlyDM == 1){
		if(((MNStruct *)globalcontext)->incDM == 0){ SignalVec = new double[((MNStruct *)globalcontext)->numProfileEpochs]();}
		sinAmp = Cube[pcount];
		pcount++;
		cosAmp = Cube[pcount];
		pcount++;
		int DMt = 0;
		for(int o=0;o<((MNStruct *)globalcontext)->numProfileEpochs; o++){
			double time=(double)((MNStruct *)globalcontext)->pulse->obsn[DMt].bat - ((MNStruct *)globalcontext)->pulse->param[param_dmepoch].val[0];
			SignalVec[o] += sinAmp*sin((2*M_PI/365.25)*time) + cosAmp*cos((2*M_PI/365.25)*time);
			DMt += ((MNStruct *)globalcontext)->numChanPerInt[o];
		}
	}
/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	      //printf("badcorr? %i %g %g \n", i,  (double)((MNStruct *)globalcontext)->pulse->obsn[i].batCorr, (double)((MNStruct *)globalcontext)->pulse->obsn[i].residual);
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;
		
	      }
	      
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr - phase - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
		//printf("loop %i %i %g %.15Lg %.15Lg \n", loop, i, Cube[0], ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr, ((MNStruct *)globalcontext)->pulse->obsn[i].residual);
	 }


	int maxshapecoeff = 0;
	int totshapecoeff = ((MNStruct *)globalcontext)->totshapecoeff; 

	int *numcoeff= new int[((MNStruct *)globalcontext)->numProfComponents];
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		numcoeff[i] =  ((MNStruct *)globalcontext)->numshapecoeff[i];
		if(debug == 1){printf("num coeff in comp %i: %i \n", i, numcoeff[i]);}
	}

	
        int *numProfileStocCoeff = ((MNStruct *)globalcontext)->numshapestoccoeff;
        int totalshapestoccoeff = ((MNStruct *)globalcontext)->totalshapestoccoeff;


	int *numEvoCoeff = ((MNStruct *)globalcontext)->numEvoCoeff;
	int totalEvoCoeff = ((MNStruct *)globalcontext)->totalEvoCoeff;

	int *numEvoFitCoeff = ((MNStruct *)globalcontext)->numEvoFitCoeff;
	int totalEvoFitCoeff = ((MNStruct *)globalcontext)->totalEvoFitCoeff;

	int *numProfileFitCoeff = ((MNStruct *)globalcontext)->numProfileFitCoeff;
	int totalProfileFitCoeff = ((MNStruct *)globalcontext)->totalProfileFitCoeff;



	int totalCoeffForMult = 0;
	int *NumCoeffForMult = new int[((MNStruct *)globalcontext)->numProfComponents];
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		NumCoeffForMult[i] = 0;
		if(numEvoCoeff[i] > NumCoeffForMult[i]){NumCoeffForMult[i]=numEvoCoeff[i];}
		if(numProfileFitCoeff[i] > NumCoeffForMult[i]){NumCoeffForMult[i]=numProfileFitCoeff[i];}
		totalCoeffForMult += NumCoeffForMult[i];
		if(debug == 1){printf("num coeff for mult from comp %i: %i \n", i, NumCoeffForMult[i]);}
	}

        int *numShapeToSave = new int[((MNStruct *)globalcontext)->numProfComponents];
        for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
                numShapeToSave[i] = numProfileStocCoeff[i];
                if(numEvoCoeff[i] >  numShapeToSave[i]){numShapeToSave[i] = numEvoCoeff[i];}
                if(numProfileFitCoeff[i] >  numShapeToSave[i]){numShapeToSave[i] = numProfileFitCoeff[i];}
                if(debug == 1){printf("saved %i %i \n", i, numShapeToSave[i]);}
        }
        int totShapeToSave = 0;
        for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
                totShapeToSave += numShapeToSave[i];
        }

	double shapecoeff[totshapecoeff];
	double StocProfCoeffs[totalshapestoccoeff];
	double EvoCoeffs[totalEvoCoeff];
	double ProfileFitCoeffs[totalProfileFitCoeff];
	double ProfileModCoeffs[totalCoeffForMult];

	for(int i =0; i < totshapecoeff; i++){
		shapecoeff[i]=((MNStruct *)globalcontext)->MeanProfileShape[i];
	}
	for(int i =0; i < totalEvoCoeff; i++){
		EvoCoeffs[i]=((MNStruct *)globalcontext)->MeanProfileEvo[i];
		//printf("loaded evo coeff %i %g \n", i, EvoCoeffs[i]);
	}
	if(debug == 1){printf("Filled %i Coeff, %i EvoCoeff \n", totshapecoeff, totalEvoCoeff);}
	profiledimstart=pcount;

	double beta = ((MNStruct *)globalcontext)->MeanProfileBeta*((MNStruct *)globalcontext)->ReferencePeriod;

	int cpos = 0;
	int epos = 0;
	int fpos = 0;
	for(int i =0; i < totalshapestoccoeff; i++){
		StocProfCoeffs[i]= pow(10.0, 2*Cube[pcount]);
		pcount++;
	}
	//printf("TotalProfileFit: %i \n",totalProfileFitCoeff );
	for(int i =0; i < totalProfileFitCoeff; i++){
	//	printf("ProfileFit: %i %i %g \n", i, pcount, Cube[pcount]);
		ProfileFitCoeffs[i]= Cube[pcount];
		pcount++;
	}
	double LinearProfileWidth=0;
	if(((MNStruct *)globalcontext)->FitLinearProfileWidth == 1){
		LinearProfileWidth = Cube[pcount];
		pcount++;
	}


	int  EvoFreqExponent = 1;
	if(((MNStruct *)globalcontext)->FitEvoExponent == 1){
		EvoFreqExponent =  floor(Cube[pcount]);
		//printf("Evo Exp set: %i %i \n", pcount, EvoFreqExponent);
		pcount++;
	}
	
	
	for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
		for(int i =0; i < numEvoFitCoeff[c]; i++){
			EvoCoeffs[i+cpos] += Cube[pcount];
			pcount++;
		}
		cpos += numEvoCoeff[c];
		
	}

	double EvoProfileWidth=0;
	if(((MNStruct *)globalcontext)->incProfileEvo == 2){
		EvoProfileWidth = Cube[pcount];
		pcount++;
	}

	if(totshapecoeff+1>=totalshapestoccoeff+1){
		maxshapecoeff=totshapecoeff+1;
	}
	if(totalshapestoccoeff+1 > totshapecoeff+1){
		maxshapecoeff=totalshapestoccoeff+1;
	}

//	printf("max shape coeff: %i \n", maxshapecoeff);
/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	double modelflux=0;
	cpos = 0;
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		for(int j =0; j < numcoeff[i]; j=j+2){
			modelflux+=sqrt(sqrt(M_PI))*sqrt(beta)*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Binomial[j])*shapecoeff[cpos+j];
		}
		cpos+= numcoeff[i];
	}
	if(debug == 1){printf("model flux: %g \n", modelflux);}
	double ModModelFlux = modelflux;
	double OPLevel = ((MNStruct *)globalcontext)->offPulseLevel;


	double lnew = 0;
	int badshape = 0;

	int GlobalNBins = (int)((MNStruct *)globalcontext)->ProfileInfo[0][1];


	double **AllHermiteBasis =  new double*[GlobalNBins];
	double **JitterBasis  =  new double*[GlobalNBins];
	double **Hermitepoly =  new double*[GlobalNBins];

	for(int i =0;i<GlobalNBins;i++){AllHermiteBasis[i]=new double[maxshapecoeff];}
	for(int i =0;i<GlobalNBins;i++){Hermitepoly[i]=new double[totshapecoeff];}
	for(int i =0;i<GlobalNBins;i++){JitterBasis[i]=new double[totshapecoeff];}



	int ProfileBaselineTerms = ((MNStruct *)globalcontext)->ProfileBaselineTerms;
	int Msize = 1+ProfileBaselineTerms+2*ProfileNoiseCoeff+totalshapestoccoeff;
	int Mcounter=0;
	if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
		Msize++;
	}
	if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
		Msize++;
	}
	if(((MNStruct *)globalcontext)->incWidthJitter > 0){
		Msize++;
	}

	double *M = new double[GlobalNBins*Msize];
	double *NM = new double[GlobalNBins*Msize];




//	for(int i =0; i < GlobalNBins; i++){
//		Mcounter=0;
//		M[i] = new double[Msize];
//		NM[i] = new double[Msize];
//
//	}


	double *MNM = new double[Msize*Msize];
//	for(int i =0; i < Msize; i++){
//	    MNM[i] = new double[Msize];
//	}


		//gettimeofday(&tval_after, NULL);
		//timersub(&tval_after, &tval_before, &tval_resultone);
		//gettimeofday(&tval_before, NULL);
	
	//for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
	
	int minep = 0;
	int maxep = ((MNStruct *)globalcontext)->numProfileEpochs;

	int t = 0;
	if(((MNStruct *)globalcontext)->SubIntToFit != -1){
		minep = ((MNStruct *)globalcontext)->SubIntToFit;
		maxep = minep+1;
		for(int ep = 0; ep < minep; ep++){	
			t += ((MNStruct *)globalcontext)->numChanPerInt[ep];
		}
	}
	int nTOA = 0;

	for(int ep = minep; ep < maxep; ep++){
	
		if(debug == 1)printf("In epoch %i \n", ep);	

		double *EpochM;
		double *EpochMNM;
		double *EpochdNM;
		double *EpochTempdNM;
		int EpochMsize = 0;

		int NChanInEpoch = ((MNStruct *)globalcontext)->numChanPerInt[ep];
		int NEpochBins = NChanInEpoch*GlobalNBins;
		double *profilenoise = new double[GlobalNBins];
		double *Bandprofilenoise = new double[NChanInEpoch*GlobalNBins];
		int *NumBinsArray = new int[NChanInEpoch];

		if(((MNStruct *)globalcontext)->incWideBandNoise == 1){

			EpochMsize = (1+ProfileBaselineTerms)*NChanInEpoch+totalshapestoccoeff;
			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
				EpochMsize++;
			}
			if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
				EpochMsize++;
			}
			if(((MNStruct *)globalcontext)->incWidthJitter > 0){
				EpochMsize++;
			}

			EpochM = new double[EpochMsize*NChanInEpoch*GlobalNBins]();

			EpochMNM = new double[EpochMsize*EpochMsize]();

			EpochdNM = new double[EpochMsize]();
			EpochTempdNM = new double[EpochMsize]();
		}

		double EpochChisq = 0;	
		double EpochdetN = 0;
		double EpochLike = 0;
		double epochstocsn=0;

		for(int ch = 0; ch < NChanInEpoch; ch++){

	
			//gettimeofday(&tval_before, NULL);

			if(debug == 1)printf("In toa %i \n", t);
			nTOA = t;

			double detN  = 0;
			double Chisq  = 0;
			double logMargindet = 0;
			double Marginlike = 0;	 
			double JitterPrior = 0;	   

			double profilelike=0;

			long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
			long double FoldingPeriodDays = FoldingPeriod/SECDAY;
			int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
			double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
			double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
			long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

			double *Betatimes = new double[Nbins];
	

			double *shapevec  = new double[Nbins];
			double *Jittervec = new double[Nbins];
			double *ProfileModVec = new double[Nbins]();
			double *ProfileJitterModVec = new double[Nbins]();
		

			double noisemean=0;
			double MLSigma = 0;
			double dataflux = 0;


			double maxamp = 0;
			double maxshape=0;

		    
			long double binpos = ModelBats[nTOA];

			if(((MNStruct *)globalcontext)->incDM==5){
	                	double DMKappa = 2.410*pow(10.0,-16);
        		        double DMScale = 1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[t].freqSSB,2));
				//printf("DMSig %i %g\n", t, SignalVec[ep]*DMScale);
				long double DMshift = (long double)(SignalVec[ep]*DMScale);
				binpos+=DMshift/SECDAY;
			}
			if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

			long double minpos = binpos - FoldingPeriodDays/2;
			if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
			long double maxpos = binpos + FoldingPeriodDays/2;
			if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];



			int InterpBin = 0;
			double FirstInterpTimeBin = 0;
			int  NumWholeBinInterpOffset = 0;

			if(((MNStruct *)globalcontext)->InterpolateProfile == 1){

		
				long double timediff = 0;
				long double bintime = ProfileBats[t][0];


				if(bintime  >= minpos && bintime <= maxpos){
				    timediff = bintime - binpos;
				}
				else if(bintime < minpos){
				    timediff = FoldingPeriodDays+bintime - binpos;
				}
				else if(bintime > maxpos){
				    timediff = bintime - FoldingPeriodDays - binpos;
				}

				timediff=timediff*SECDAY;

				double OneBin = FoldingPeriod/Nbins;
				int NumBinsInTimeDiff = floor(timediff/OneBin + 0.5);
				double WholeBinsInTimeDiff = NumBinsInTimeDiff*FoldingPeriod/Nbins;
				double OneBinTimeDiff = -1*((double)timediff - WholeBinsInTimeDiff);

				double PWrappedTimeDiff = (OneBinTimeDiff - floor(OneBinTimeDiff/OneBin)*OneBin);

				if(debug == 1)printf("Making InterpBin: %g %g %i %g %g %g\n", (double)timediff, OneBin, NumBinsInTimeDiff, WholeBinsInTimeDiff, OneBinTimeDiff, PWrappedTimeDiff);

				InterpBin = floor(PWrappedTimeDiff/((MNStruct *)globalcontext)->InterpolatedTime+0.5);
				if(InterpBin >= ((MNStruct *)globalcontext)->NumToInterpolate)InterpBin -= ((MNStruct *)globalcontext)->NumToInterpolate;

				FirstInterpTimeBin = -1*(InterpBin-1)*((MNStruct *)globalcontext)->InterpolatedTime;

				if(debug == 1)printf("Interp Time Diffs: %g %g %g %g \n", ((MNStruct *)globalcontext)->InterpolatedTime, InterpBin*((MNStruct *)globalcontext)->InterpolatedTime, PWrappedTimeDiff, InterpBin*((MNStruct *)globalcontext)->InterpolatedTime-PWrappedTimeDiff);

				double FirstBinOffset = timediff-FirstInterpTimeBin;
				double dNumWholeBinOffset = FirstBinOffset/(FoldingPeriod/Nbins);
				int  NumWholeBinOffset = 0;

				NumWholeBinInterpOffset = floor(dNumWholeBinOffset+0.5);
				NumBinsArray[ch] = NumWholeBinInterpOffset;
	
				if(debug == 1)printf("Interp bin is: %i , First Bin is %g, Offset is %i \n", InterpBin, FirstInterpTimeBin, NumWholeBinInterpOffset);


			}
		   
			if(((MNStruct *)globalcontext)->InterpolateProfile == 0){

				for(int j =0; j < Nbins; j++){


					long double timediff = 0;
					long double bintime = ProfileBats[t][j];
					

					if(bintime  >= minpos && bintime <= maxpos){
					    timediff = bintime - binpos;
					}
					else if(bintime < minpos){
					    timediff = FoldingPeriodDays+bintime - binpos;
					}
					else if(bintime > maxpos){
					    timediff = bintime - FoldingPeriodDays - binpos;
					}


					timediff=timediff*SECDAY;


					Betatimes[j]=timediff/beta;
					TNothpl(maxshapecoeff,Betatimes[j],AllHermiteBasis[j]);

					for(int k =0; k < maxshapecoeff; k++){
						double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));
						AllHermiteBasis[j][k]=AllHermiteBasis[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);

						if(k<totshapecoeff){ Hermitepoly[j][k] = AllHermiteBasis[j][k];}

					}

					JitterBasis[j][0] = (1.0/sqrt(2.0))*(-1.0*Hermitepoly[j][1]);
					for(int k =1; k < totshapecoeff; k++){
						JitterBasis[j][k] = (1.0/sqrt(2.0))*(sqrt(double(k))*AllHermiteBasis[j][k-1] - sqrt(double(k+1))*AllHermiteBasis[j][k+1]);
					}

				}

				dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,totshapecoeff,'N');
				dgemv(JitterBasis,shapecoeff,Jittervec,Nbins,totshapecoeff,'N');


			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//////////////////////////////////////////////////////Get Noise Level//////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



				maxshape=0;
				for(int j =0; j < Nbins; j++){
					if(shapevec[j] > maxshape){ maxshape = shapevec[j];}
				}
				noisemean=0;
				int numoffpulse = 0;
				for(int j = 0; j < Nbins; j++){
					if(shapevec[j] < maxshape*OPLevel){
						noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
						numoffpulse += 1;
					}
						
				}

				if(debug == 1)printf("noise mean %g num off pulse %i\n", noisemean, numoffpulse);
				noisemean = noisemean/(numoffpulse);

				MLSigma = 0;
				int MLSigmaCount = 0;
				dataflux = 0;
				for(int j = 0; j < Nbins; j++){
					if(shapevec[j] < maxshape*OPLevel){
						double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean;
						MLSigma += res*res; MLSigmaCount += 1;
					}
					else{
						if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean)/MLSigma > 1){
							dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-noisemean)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
						}
					}
				}

				MLSigma = sqrt(MLSigma/MLSigmaCount);
				MLSigma = MLSigma*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];
				maxamp = dataflux/modelflux;
				if(debug == 1)printf("noise %g, dataflux %g, modelflux %g, maxamp %g \n", MLSigma, dataflux, modelflux, maxamp);

		                for(int j =0; j < Nbins; j++){

				
					M[j] = 1;
					M[j + Nbins] = shapevec[j];

					Mcounter = 2;

					if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
						M[j + Mcounter*Nbins] = Jittervec[j]*dataflux/modelflux/beta;
						Mcounter++;
					}

					if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
						double DMKappa = 2.410*pow(10.0,-16);
						double DMScale = 1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[t].freqSSB,2));
						M[j + Mcounter*Nbins] = Jittervec[j]*DMScale*dataflux/modelflux/beta;
						Mcounter++;
					}

					int stocpos = 0;
                                        int savedpos=0;
                                        for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
                                                for(int k = 0; k < numProfileStocCoeff[c]; k++){
                                                    M[j + (Mcounter+k+stocpos)*Nbins] = AllHermiteBasis[j][k+savedpos]*dataflux;
                                                }
                                                stocpos+=numProfileStocCoeff[c];
                                                savedpos+=numShapeToSave[c];
                                        }




				}
			}

	//gettimeofday(&tval_after, NULL);
	//timersub(&tval_after, &tval_before, &tval_resultone);
	//printf("Time elapsed Up to Evo: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
	//gettimeofday(&tval_before, NULL);


			double reffreq = ((MNStruct *)globalcontext)->EvoRefFreq;
			double freqdiff =  (((MNStruct *)globalcontext)->pulse->obsn[t].freq - reffreq)/1000.0;
			double freqscale = pow(freqdiff, EvoFreqExponent);

			//printf("Evo Exp %i %i %g %g \n", t, EvoFreqExponent, freqdiff, freqscale);

			if(((MNStruct *)globalcontext)->InterpolateProfile == 1){

			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//////////////////////////////////////////////////////Add in any Profile Changes///////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				for(int i =0; i < totalCoeffForMult; i++){
					ProfileModCoeffs[i]=0;	
				}				

				cpos = 0;
				epos = 0;
				fpos = 0;
				for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
					
					for(int i =0; i < numProfileFitCoeff[c]; i++){
						ProfileModCoeffs[i+cpos] += ProfileFitCoeffs[i+fpos];
						//if(t==0){printf("Mod FitCoeff: %i %i %g \n", i, fpos,  ProfileFitCoeffs[i+fpos]);}
					}

					for(int i =0; i < numEvoCoeff[c]; i++){
						ProfileModCoeffs[i+cpos] += EvoCoeffs[i+epos]*freqscale;
						//if(t==0){printf("Mod EvoCoeff: %i %i %g \n", i, epos,   EvoCoeffs[i+epos]);}
					}

					cpos += NumCoeffForMult[c];
					epos += numEvoCoeff[c];
					fpos += numProfileFitCoeff[c];	
				}


				

				if(totalCoeffForMult > 0){
					vector_dgemv(((MNStruct *)globalcontext)->InterpolatedShapeletsVec[InterpBin], ProfileModCoeffs,ProfileModVec,Nbins,totalCoeffForMult,'N');

					if(((MNStruct *)globalcontext)->numFitEQUAD > 0 || ((MNStruct *)globalcontext)->incDMEQUAD > 0 ){
						vector_dgemv(((MNStruct *)globalcontext)->InterpolatedJitterProfileVec[InterpBin], ProfileModCoeffs,ProfileJitterModVec,Nbins,totalCoeffForMult,'N');
					}
				}
				ModModelFlux = modelflux;

				cpos=0;
				for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
					for(int j =0; j < NumCoeffForMult[c]; j=j+2){
						ModModelFlux+=sqrt(sqrt(M_PI))*sqrt(beta)*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Binomial[j])*ProfileModCoeffs[cpos+j];
					}
					cpos+= NumCoeffForMult[c];
				}


	//gettimeofday(&tval_after, NULL);
	//timersub(&tval_after, &tval_before, &tval_resultone);
	//printf("Time elapsed,  Evo: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
	//gettimeofday(&tval_before, NULL);


			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//////////////////////////////////////////////////////Fill Arrays with interpolated state//////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				noisemean=0;
				int numoffpulse = 0;
		                maxshape=0;
				
				for(int j =0; j < Nbins; j++){

					double NewIndex = (j + NumWholeBinInterpOffset);
					int Nj =  Wrap(j + NumWholeBinInterpOffset, 0, Nbins-1);//(int)(NewIndex - floor(NewIndex/Nbins)*Nbins);

					double widthTerm = ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpBin][Nj]*LinearProfileWidth; 
					double evoWidthTerm = ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpBin][Nj]*EvoProfileWidth*freqscale;

					shapevec[j] = ((MNStruct *)globalcontext)->InterpolatedMeanProfile[InterpBin][Nj] + widthTerm + ProfileModVec[Nj] + evoWidthTerm;

					//if(t==0){printf("fill evo coeff %i %g %g %g %g \n", j, ProfileModVec[Nj], widthTerm, evoWidthTerm,((MNStruct *)globalcontext)->InterpolatedMeanProfile[InterpBin][Nj]);}

					Mcounter = 0;
					for(int q = 0; q < ProfileBaselineTerms; q++){
						M[j+Mcounter*Nbins] = pow(double(j)/Nbins, q);
						Mcounter++;
						//printf("Baseline: %i %i %i %g \n", t, j, q, M[0]);
					}

					M[j + Mcounter*Nbins] = shapevec[j];

					if(shapevec[j] > maxshape){ maxshape = shapevec[j];}
					Mcounter++;
				
					for(int q = 0; q < ProfileNoiseCoeff; q++){

						M[j+Mcounter*Nbins] =     cos(2*M_PI*(double(q+1)/Nbins)*j);
						M[j+(Mcounter+1)*Nbins] = sin(2*M_PI*(double(q+1)/Nbins)*j);
//						if(t==0){printf("%i %g %g %g \n", q, 1.0/(q+1), cos(2*M_PI*(1.0/(q+1))*j), sin(2*M_PI*(1.0/(q+1))*j));}
						Mcounter += 2;
					}

					if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
						M[j + Mcounter*Nbins] = ((MNStruct *)globalcontext)->InterpolatedJitterProfile[InterpBin][Nj] + ProfileJitterModVec[Nj];

						Mcounter++;
					}		

					if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
						double DMKappa = 2.410*pow(10.0,-16);
						double DMScale = 1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[t].freqSSB,2));
						M[j + Mcounter*Nbins] = (((MNStruct *)globalcontext)->InterpolatedJitterProfile[InterpBin][Nj] + ProfileJitterModVec[Nj])*DMScale;
						Mcounter++;
					}	


					if(((MNStruct *)globalcontext)->incWidthJitter > 0){
						M[j + Mcounter*Nbins] = ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpBin][Nj];
						Mcounter++;
					}

                                        int stocpos=0;
                                        int savepos=0;
                                        for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
                                                for(int k = 0; k < numProfileStocCoeff[c]; k++){
                                                    M[j + (Mcounter+k+stocpos)*Nbins] = ((MNStruct *)globalcontext)->InterpolatedShapeletsVec[InterpBin][Nj + (k+savepos)*Nbins];
                                                }
                                                stocpos+=numProfileStocCoeff[c];
                                                savepos+=numShapeToSave[c];

                                        }


					//printf("toa: %i %i %g %g %g %g\n", nTOA, j, ((MNStruct *)globalcontext)->InterpolatedMeanProfile[InterpBin][Nj] , widthTerm , ProfileModVec[Nj], ((MNStruct *)globalcontext)->MaxShapeAmp*OPLevel);
						
				}


				for(int j =0; j < Nbins; j++){
					if(shapevec[j] < maxshape*OPLevel){
						noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
						numoffpulse += 1;
						//if(nTOA==4)printf("off pulse: %i %g %g\n", j, (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1], shapevec[j]);
					}	
				}

				if(debug == 1)printf("noise mean %g num off pulse %i\n", noisemean, numoffpulse);

	//gettimeofday(&tval_after, NULL);
	//timersub(&tval_after, &tval_before, &tval_resultone);
	//printf("Time elapsed,  FillArrays: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
	//gettimeofday(&tval_before, NULL);

			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//////////////////////////////////////////////////////Get Noise Level//////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				noisemean = noisemean/(numoffpulse);

				MLSigma = 0;
				int MLSigmaCount = 0;
				dataflux = 0;
				for(int j = 0; j < Nbins; j++){
					if(shapevec[j] < maxshape*OPLevel){
						double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean;
						MLSigma += res*res; MLSigmaCount += 1;
					}
				}

				MLSigma = sqrt(MLSigma/MLSigmaCount);
				MLSigma = MLSigma*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];

				dataflux = 0;
				for(int j = 0; j < Nbins; j++){
					if(shapevec[j] >= maxshape*OPLevel){
						if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean)/MLSigma > 1){
							dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-noisemean)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
						}
					}
				}

				maxamp = dataflux/ModModelFlux;

				if(debug == 1)printf("noise %g, dataflux %g, modelflux %g, maxamp %g \n", MLSigma, dataflux, ModModelFlux, maxamp);

	//gettimeofday(&tval_after, NULL);
	//timersub(&tval_after, &tval_before, &tval_resultone);
	//printf("Time elapsed,  Get Noise: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
	//gettimeofday(&tval_before, NULL);

			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//////////////////////////////////////////////////////Rescale Basis Vectors////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


				Mcounter = 1+ProfileBaselineTerms+ProfileNoiseCoeff*2;
				if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
					for(int j =0; j < Nbins; j++){
						M[j + Mcounter*Nbins] = M[j + Mcounter*Nbins]*dataflux/ModModelFlux/beta;
					}			
					Mcounter++;
				}
				if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
					for(int j =0; j < Nbins; j++){
						M[j + Mcounter*Nbins] = M[j + Mcounter*Nbins]*dataflux/ModModelFlux/beta;
					}			
					Mcounter++;
				}
				if(((MNStruct *)globalcontext)->incWidthJitter > 0){
					for(int j =0; j < Nbins; j++){
						M[j + Mcounter*Nbins] = M[j + Mcounter*Nbins]*dataflux/ModModelFlux;
					}
					Mcounter++;
				}

				for(int k = 0; k < totalshapestoccoeff; k++){
					for(int j =0; j < Nbins; j++){
				   		M[j + (Mcounter+k)*Nbins] = M[j + (Mcounter+k)*Nbins]*dataflux;
					}
				}

				if(((MNStruct *)globalcontext)->incWideBandNoise == 1){
					for(int j =0; j < Nbins; j++){
						for(int k = 0; k < 1+ProfileBaselineTerms; k++){
							EpochM[ch*Nbins+j +    (ch*(1+ProfileBaselineTerms)+k)*NChanInEpoch*Nbins]  = M[j + k*Nbins];
						}
						for(int k = 0; k < Msize-(1+ProfileBaselineTerms); k++){
							EpochM[ch*Nbins+j + ((1+ProfileBaselineTerms)*NChanInEpoch+k)*NChanInEpoch*Nbins] = M[j + (1+ProfileBaselineTerms+k)*Nbins];
						}
					}
				}



		    }

	
		    double *NDiffVec = new double[Nbins];

	//	    double maxshape=0;
	//	    for(int j =0; j < Nbins; j++){
	// 	        if(shapevec[j] > maxshape){ maxshape = shapevec[j];}
	//	    }

	
		///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


			      
			Chisq = 0;
			detN = 0;
			double OffPulsedetN = log(MLSigma*MLSigma);
			double OneDetN = 0;
			double logconvfactor = 1.0/log2(exp(1));

	
	 
			double highfreqnoise = HighFreqStoc1;
			double flatnoise = (HighFreqStoc2*maxamp*maxshape)*(HighFreqStoc2*maxamp*maxshape);
			
			
			for(int i =0; i < Nbins; i++){
			  	Mcounter=2;
				double noise = MLSigma*MLSigma;

				OneDetN=OffPulsedetN;
				if(shapevec[i] > maxshape*OPLevel && ((MNStruct *)globalcontext)->incHighFreqStoc > 0){
					noise +=  flatnoise + (highfreqnoise*maxamp*shapevec[i])*(highfreqnoise*maxamp*shapevec[i]);
					OneDetN=log2(noise)*logconvfactor;
				}

				detN += OneDetN;
				profilenoise[i] = sqrt(noise);
				Bandprofilenoise[ch*Nbins + i] = sqrt(noise);
				noise=1.0/noise;
				
				double datadiff =  ((MNStruct *)globalcontext)->ProfileData[nTOA][i][1];
//				printf("Data: %i %g %g \n",i,  datadiff, noise);
				NDiffVec[i] = datadiff*noise;

				Chisq += datadiff*NDiffVec[i];
	

				
				for(int j = 0; j < Msize; j++){
					NM[i + j*Nbins] = M[i + j*Nbins]*noise;
				}

				

			}


	//gettimeofday(&tval_after, NULL);
	//timersub(&tval_after, &tval_before, &tval_resultone);
	//printf("Time elapsed,  Up to LA: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
	//gettimeofday(&tval_before, NULL);

			vector_dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');


			double *dNM = new double[Msize];
			double *TempdNM = new double[Msize];
			vector_dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		
			if(((MNStruct *)globalcontext)->incWideBandNoise == 1){

				int ppterms = 1+ProfileBaselineTerms;

				for(int j = 0; j < ppterms; j++){
					EpochdNM[ppterms*ch+j] = dNM[j];
					for(int k = 0; k < ppterms; k++){
						EpochMNM[ppterms*ch+j + (ppterms*ch+k)*EpochMsize] = MNM[j + k*Msize];
					}
				}

				for(int j = 0; j < Msize-ppterms; j++){

					EpochdNM[ppterms*NChanInEpoch+j] += dNM[ppterms+j];

				//	for(int k = 0; k < ppterms; k++){

						for(int q = 0; q < ppterms; q++){
							EpochMNM[ppterms*ch+q + (ppterms*NChanInEpoch+j)*EpochMsize] = MNM[q + (ppterms+j)*Msize];
							//EpochMNM[ppterms*ch+1 + (ppterms*NChanInEpoch+j)*EpochMsize] = MNM[1 + (ppterms+j)*Msize];

							EpochMNM[ppterms*NChanInEpoch+j + (ppterms*ch+q)*EpochMsize] = MNM[ppterms+j + q*Msize];
							//EpochMNM[ppterms*NChanInEpoch+j + (ppterms*ch+1)*EpochMsize] = MNM[ppterms+j + 1*Msize];
						}
				//	}

					for(int k = 0; k < Msize-ppterms; k++){

						EpochMNM[ppterms*NChanInEpoch+j + (ppterms*NChanInEpoch+k)*EpochMsize] += MNM[ppterms+j + (ppterms+k)*Msize];
					}
				}
			}


	

			double Margindet = 0;
			double StocProfDet = 0;
			double JitterDet = 0;
			if(((MNStruct *)globalcontext)->incWideBandNoise == 0){


				Mcounter=1+ProfileBaselineTerms;

				if(((MNStruct *)globalcontext)->incProfileNoise == 1 || ((MNStruct *)globalcontext)->incProfileNoise == 2){
					for(int q = 0; q < ProfileNoiseCoeff; q++){
						double profnoise = ProfileNoiseAmps[q];
						JitterDet += 2*log(profnoise);
						MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
						Mcounter++;
						MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
						Mcounter++;
					}
				}

				if(((MNStruct *)globalcontext)->incProfileNoise == 3 || ((MNStruct *)globalcontext)->incProfileNoise == 4){
					for(int q = 0; q < ProfileNoiseCoeff; q++){
						double profnoise = ProfileNoiseAmps[q]*((MNStruct *)globalcontext)->MLProfileNoise[ep][q+1];
						JitterDet += 2*log(profnoise);
						MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
						Mcounter++;
						MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
						Mcounter++;
					}
				}
				if(((MNStruct *)globalcontext)->numFitEQUAD > 0){

					double profnoise = EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
					profnoise = profnoise*profnoise;
				    	JitterDet += log(profnoise);
				    	MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
					Mcounter++;
				}

				if(((MNStruct *)globalcontext)->incDMEQUAD > 0){

					double profnoise = DMEQUAD;
					profnoise = profnoise*profnoise;
				    	JitterDet += log(profnoise);
				    	MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
					Mcounter++;
				}


				if(((MNStruct *)globalcontext)->incWidthJitter > 0){

					double profnoise = WidthJitter;
					profnoise = profnoise*profnoise;
				    	JitterDet += log(profnoise);
				    	MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
					Mcounter++;
				}
				
				for(int j = 0; j < totalshapestoccoeff; j++){
					StocProfDet += log(StocProfCoeffs[j]);
					MNM[Mcounter+j + (Mcounter+j)*Msize] +=  1.0/StocProfCoeffs[j];
				}


				for(int i =0; i < Msize; i++){
					TempdNM[i] = dNM[i];
				}


				int info=0;
				
				vector_dpotrfInfo(MNM, Msize, Margindet, info);
				vector_dpotrsInfo(MNM, TempdNM, Msize, info);
					    
				logMargindet = Margindet;

				Marginlike = 0;
				for(int i =0; i < Msize; i++){
					Marginlike += TempdNM[i]*dNM[i];
					//printf("one dnm: %i %i %g\n", ep, ch, TempdNM[i]);
				}


				double finaloffset = TempdNM[0];
				double finalamp = TempdNM[ProfileBaselineTerms];
				double finalJitter = 0;
				double finalWidthJitter = 0;
				if(((MNStruct *)globalcontext)->numFitEQUAD > 0)finalJitter = TempdNM[1+ProfileBaselineTerms];
				if(((MNStruct *)globalcontext)->incWidthJitter > 0){
					finalWidthJitter = TempdNM[1+ProfileBaselineTerms];
					printf("FWJ: %i %g\n", t, finalWidthJitter);
				}
				double *StocVec = new double[Nbins];
		
				for(int i =0; i < Nbins; i++){
					if(((MNStruct *)globalcontext)->numFitEQUAD > 0)Jittervec[i] = M[i + (1+ProfileBaselineTerms)*Nbins];
					if(((MNStruct *)globalcontext)->numFitEQUAD == 0)Jittervec[i] = 0;
//					if(t==0){printf("p: %i %g %g %g  %g\n", i, M[i+0*Nbins], M[i+1*Nbins], M[i+2*Nbins], M[i+3*Nbins]);}
				}

				vector_dgemv(M,TempdNM,shapevec,Nbins,Msize,'N');

		
				for(int q = 0; q < 1+ProfileBaselineTerms; q++){
					TempdNM[q] = 0;
				}
				if(((MNStruct *)globalcontext)->numFitEQUAD > 0)TempdNM[1+ProfileBaselineTerms] = 0;
		

				vector_dgemv(M,TempdNM,StocVec,Nbins,Msize,'N');

				double StocSN = 0;
				double DataStocSN = 0;

				if(writeprofiles == 1){
					std::ofstream profilefile;
					char toanum[15];
					sprintf(toanum, "%d", nTOA);
					std::string ProfileName =  toanum;
					std::string dname = longname+ProfileName+"-Profile.txt";
	
					profilefile.open(dname.c_str());
					double MLChisq = 0;
					profilefile << "#" << std::setprecision(20) << ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0] << "\n";
					for(int i =0; i < Nbins; i++){
						StocSN += StocVec[i]*StocVec[i]/profilenoise[i]/profilenoise[i];
					}

					for(int i =0; i < Nbins; i++){
						int Nj =  Wrap(i+Nbins/2 + NumWholeBinInterpOffset, 0, Nbins-1);
						//if(ep==0 && StocSN > 1){printf("%i %i %i %i %g %g %g %g %g\n", ep, ch, i, Nj, (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1], finaloffset + finalamp*M[i + 1*Nbins], profilenoise[i], StocSN, StocVec[i]);}
						
						MLChisq += pow(((double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1] - shapevec[i])/profilenoise[i], 2);
						profilefile << Nj << " " << std::setprecision(10) << (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1] << " " << shapevec[i] << " " << profilenoise[i] << " " << finaloffset+finalamp*M[i + ProfileBaselineTerms*Nbins] << " " << finalJitter*Jittervec[i] << " " << StocVec[i] << " " << M[i + ProfileBaselineTerms*Nbins] << "\n";
		//				if(fabs((double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1] - shapevec[i])/profilenoise[i] > 5)printf("5 sig %i %i \n", t, i);
	//					if(t==0){printf("p: %i %i %g %g %g  %g\n", ProfileBaselineTerms, i, M[i+0*Nbins], M[i+1*Nbins], M[i+2*Nbins], M[i+3*Nbins]);}

					}
			    		profilefile.close();
					//if(ep==0){printf("Stoc SN: %i %g \n", t, sqrt(StocSN));}
					epochstocsn+=StocSN;
				}
				
				delete[] StocVec;


			}


			
			
			profilelike = -0.5*(detN + Chisq + logMargindet + StocProfDet + JitterDet - Marginlike);
			//lnew += profilelike;
			if(debug == 1)printf("Like: %i %.15g %.15g %.15g %.15g %.15g %.15g %.15g\n", nTOA,lnew, detN, Chisq, logMargindet, Marginlike, StocProfDet , JitterDet);

			EpochChisq+=Chisq;
			EpochdetN+=detN;
			EpochLike+=profilelike;

			delete[] shapevec;
			delete[] Jittervec;
			delete[] ProfileModVec;
			delete[] ProfileJitterModVec;
			delete[] NDiffVec;
			delete[] dNM;
			delete[] TempdNM;
			delete[] Betatimes;


	//gettimeofday(&tval_after, NULL);
	//timersub(&tval_after, &tval_before, &tval_resultone);
	//printf("Time elapsed,  LA: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);

			t++;

	
		}
		//printf("Epoch SN: %i %g \n", ep, epochstocsn);
		if(((MNStruct *)globalcontext)->incWideBandNoise == 0){

			delete[] profilenoise;		
		}

///////////////////////////////////////////


		if(((MNStruct *)globalcontext)->incWideBandNoise == 0){

			lnew += EpochLike;
		}

		if(((MNStruct *)globalcontext)->incWideBandNoise == 1){

			for(int i =0; i < EpochMsize; i++){
				EpochTempdNM[i] = EpochdNM[i];
			}

			int EpochMcount=0;
			if(((MNStruct *)globalcontext)->incProfileNoise == 0){EpochMcount = (1+ProfileBaselineTerms)*NChanInEpoch;}

			double BandJitterDet = 0;

			if(((MNStruct *)globalcontext)->incProfileNoise == 3 || ((MNStruct *)globalcontext)->incProfileNoise == 4){
				for(int ch = 0; ch < ((MNStruct *)globalcontext)->numChanPerInt[ep]; ch++){
					
					EpochMcount += 1+ProfileBaselineTerms;
					for(int q = 0; q < ProfileNoiseCoeff; q++){
						if(debug == 1){printf("PN BW %i %i %i %i %g %g \n", ep, ch, EpochMcount, q, EpochMNM[EpochMcount + EpochMcount*EpochMsize], EpochMNM[EpochMcount+1 + (EpochMcount+1)*EpochMsize]);}
						double profnoise = ((MNStruct *)globalcontext)->MLProfileNoise[ep][q+1];
						BandJitterDet += 2*log(profnoise);
						EpochMNM[EpochMcount + EpochMcount*EpochMsize] += 1.0/profnoise;
						EpochMcount++;
						EpochMNM[EpochMcount + EpochMcount*EpochMsize] += 1.0/profnoise;
						EpochMcount++;
					}
				}
			}

			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){

				double profnoise = EQUAD[((MNStruct *)globalcontext)->sysFlags[0]];
				profnoise = profnoise*profnoise;
			    	BandJitterDet += log(profnoise);

				EpochMNM[EpochMcount + (EpochMcount)*EpochMsize] += 1.0/profnoise;
				EpochMcount++;
			}

			if(((MNStruct *)globalcontext)->incDMEQUAD > 0){

				double profnoise = DMEQUAD;
				profnoise = profnoise*profnoise;
			    	BandJitterDet += log(profnoise);

				EpochMNM[EpochMcount + (EpochMcount)*EpochMsize] += 1.0/profnoise;
				EpochMcount++;
			}

			if(((MNStruct *)globalcontext)->incWidthJitter > 0){

				double profnoise = WidthJitter;
				profnoise = profnoise*profnoise;
			    	BandJitterDet += log(profnoise);

				EpochMNM[EpochMcount + (EpochMcount)*EpochMsize] += 1.0/profnoise;
				EpochMcount++;
			}
/*
			double BandStocProfDet = 0;
			for(int j = 0; j < totalshapestoccoeff; j++){
				BandStocProfDet += log(StocProfCoeffs[j]);
			if(writeprofiles == 1){
				for(int j = 0; j < NChanInEpoch; j++){
					double StocSN = 0;
					double ProfSN = 0;
					currentnToA++;
					std::ofstream profilefile;
					char toanum[15];
					
					sprintf(toanum, "%d", currentnToA);
					std::string ProfileName =  toanum;
					std::string dname = longname+ProfileName+"-Profile.txt";

					profilefile.open(dname.c_str());
					double MLChisq = 0;

					for(int i =0; i < GlobalNBins; i++){
						int Nj =  Wrap(i+GlobalNBins/2 + NumBinsArray[j], 0, GlobalNBins-1);

						StocSN += StocVec[j*GlobalNBins + i]*StocVec[j*GlobalNBins + i];
						ProfSN += finalamps[j]*EpochM[j*GlobalNBins+i +(j*(1+ProfileBaselineTerms)+1)*NChanInEpoch*GlobalNBins]*finalamps[j]*EpochM[j*GlobalNBins+i +(j*(1+ProfileBaselineTerms)+1)*NChanInEpoch*GlobalNBins];
						MLChisq += pow(((double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1] - maxvec[j*GlobalNBins + i])/Bandprofilenoise[j*GlobalNBins + i], 2);
						profilefile << Nj  << " " << std::setprecision(10) << (double)((MNStruct *)globalcontext)->ProfileData[currentnToA][i][1] << " " << maxvec[j*GlobalNBins + i] << " " << Bandprofilenoise[j*GlobalNBins + i] << " " << finaloffsets[j] + finalamps[j]*EpochM[j*GlobalNBins+i +    (j*(1+ProfileBaselineTerms)+1)*NChanInEpoch*GlobalNBins] << " " << finalJitter*Jittervec[i] << " " << StocVec[j*GlobalNBins + i] << "\n";

					}

			    		profilefile.close();
					epochstocsn+=StocSN/ProfSN;
				}
			}

			if(debug == 1){printf("deallocating 1\n");}
			delete[] profilenoise;
			delete[] Bandprofilenoise;
			if(debug == 1){printf("deallocating 2\n");}
			delete[] StocVec;
			delete[] finaloffsets;
			if(debug == 1){printf("deallocating 3\n");}
			delete[] finalamps;
			delete[] Jittervec;
			delete[] maxvec;
			delete[] NumBinsArray;

			double EpochProfileLike = -0.5*(EpochdetN + EpochChisq + EpochMargindet - EpochMarginlike + BandJitterDet + BandStocProfDet);
			lnew += EpochProfileLike;
			if(debug == 1){printf("Like: %i %g %g %g %g %g %g \n",ep,  EpochdetN , EpochChisq ,EpochMargindet , EpochMarginlike , BandJitterDet , BandStocProfDet);}

			delete[] EpochMNM;
			delete[] EpochdNM;
			if(debug == 1){printf("deallocating 4\n");}
			delete[] EpochTempdNM;
			delete[] EpochM;

		
			if(debug == 1){printf("deallocating done\n");}
*/
		}

////////////////////////////////////////////
	
	}
	 
	 
	for (int j = 0; j < GlobalNBins; j++){
	    delete[] Hermitepoly[j];
	    delete[] JitterBasis[j];
	    delete[] AllHermiteBasis[j];
	   // delete[] M[j];
	   // delete[] NM[j];
	}
	delete[] Hermitepoly;
	delete[] AllHermiteBasis;
	delete[] JitterBasis;
	delete[] M;
	delete[] NM;

	//for (int j = 0; j < Msize; j++){
	//    delete[] MNM[j];
	//}
	delete[] MNM;
	 

	if(((MNStruct *)globalcontext)->incDM > 0){
		delete[] SignalVec;
	}
	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;
	delete[] numcoeff;
	delete[] NumCoeffForMult;
	delete[] ProfileNoiseAmps;
	lnew += uniformpriorterm - 0.5*(FreqLike + FreqDet);
	if(debug == 1)printf("End Like: %.10g \n", lnew);
	//printf("End Like: %.10g \n", lnew);
	//}

//	if(((MNStruct *)globalcontext)->incDM > 0){
		//WriteDMSignal(longname, ndim);
//	}
	//WriteProfileFreqEvo(longname, ndim,profiledimstart);


}


void  WriteDMSignal(std::string longname, int &ndim){


	int InterpIndex = 0;
        double *Cube = new double[ndim];
        int number_of_lines = 0;

        std::ifstream checkfile;
	std::string checkname;
	if(((MNStruct *)globalcontext)->sampler == 0){
	        checkname = longname+".txt";
	}
        if(((MNStruct *)globalcontext)->sampler == 1){
                checkname = longname+".txt";
        }
	//printf("%i %s \n", ((MNStruct *)globalcontext)->sampler, checkname.c_str());
        checkfile.open(checkname.c_str());
        std::string line;
        while (getline(checkfile, line))
                ++number_of_lines;

        checkfile.close();

        std::ifstream summaryfile;
	std::string fname;
	if(((MNStruct *)globalcontext)->sampler == 0){
		fname = longname+".txt";
	}
	if(((MNStruct *)globalcontext)->sampler == 1){
	        fname = longname+".txt";
	}

        summaryfile.open(fname.c_str());


	double *DMSignal = new double[((MNStruct *)globalcontext)->numProfileEpochs]();

	double *DMSignalErr = new double[((MNStruct *)globalcontext)->numProfileEpochs]();
	printf("Getting Means with %i lines \n", number_of_lines);
	double weightsum=0;
        for(int l=0;l<number_of_lines;l++){
		if(l%1000==0){printf("Line: %i \n", l);}

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);
                double weight = paramlist[0];
		weightsum+=weight;

		 for(int i = 0; i < ndim; i++){
		         Cube[i] = paramlist[i+2];
		 }


		int dotime = 0;
		struct timeval tval_before, tval_after, tval_resultone, tval_resulttwo;


		//gettimeofday(&tval_before, NULL);
		//double bluff = 0;
		//for(int loop = 0; loop < 5; loop++){
		//	Cube[0] = loop*0.2;

		int debug = 0;

		if(debug == 1)printf("In like \n");

		long double LDparams[ndim];
		int pcount;


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


		int fitcount=0;
		for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){

			LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
			fitcount++;

		}	
		
		
		pcount=0;
		long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
		pcount++;
		for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
			((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];
			pcount++;
		}
		for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
			((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
			pcount++;
		}

		long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
		for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

		   JumpVec[p] = 0;
		   for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
			for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
				if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){
					JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
				}
			 }
		   }

		}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form SATS///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


		for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
			((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
		}

		///for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
		//	((MNStruct *)globalcontext)->pulse->jumpVal[k]= 0;
		//}
	       for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		      ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
		}

		delete[] JumpVec;


//		fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);    
//		formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);     
		



		
		if(debug == 1)printf("Phase: %g \n", (double)phase);
		if(debug == 1)printf("Formed Residuals \n");

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////White noise/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


		  
		if(((MNStruct *)globalcontext)->numFitEFAC == 1){
			pcount++;
			
		}
		else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
			for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
				pcount++;
			}
		}	
		  
		if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		}
		else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
			pcount++;
		}
		else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
			for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
				pcount++;
			}
		}


		if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		}
		else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
			pcount++;
		}
		else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
			for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
				pcount++;
			}
		}	  
		  

		if(((MNStruct *)globalcontext)->incHighFreqStoc == 1){
			pcount++;
		}
		if(((MNStruct *)globalcontext)->incHighFreqStoc == 2){
			pcount++;
			pcount++;
		}

		if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
			pcount++;
		}	
		
		if(((MNStruct *)globalcontext)->incWidthJitter > 0){
			pcount++;
		}



		if(((MNStruct *)globalcontext)->incProfileNoise == 1){

			pcount++;
			pcount++;
		}

		if(((MNStruct *)globalcontext)->incProfileNoise == 2){

			for(int q = 0; q < ((MNStruct *)globalcontext)->ProfileNoiseCoeff; q++){
				pcount++;
			}
		}

		if(((MNStruct *)globalcontext)->incProfileNoise == 3){

			for(int q = 0; q < ((MNStruct *)globalcontext)->ProfileNoiseCoeff; q++){
				pcount++;
			}
		}


		 int DMt = 0;
		 for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){
			double dmDot = 0;
			longdouble yrs = (((MNStruct *)globalcontext)->pulse->obsn[DMt].bat - ((MNStruct *)globalcontext)->pulse->param[param_dmepoch].val[0])/365.25;
			longdouble arg = 1.0;

			for (int d=0;d<9;d++){
			    if(d>0){
				arg *= yrs;
			    }
			    if (((MNStruct *)globalcontext)->pulse->param[param_dm].paramSet[d]==1){
				if(d>0){
				    dmDot+=(double)(((MNStruct *)globalcontext)->pulse->param[param_dm].val[d]*arg);
				}
			    }
			}

			if (((MNStruct *)globalcontext)->pulse->param[param_dm_sin1yr].paramSet[0]==1){
			    dmDot += ((MNStruct *)globalcontext)->pulse->param[param_dm_sin1yr].val[0]*sin(2*M_PI*yrs);
			}
			if (((MNStruct *)globalcontext)->pulse->param[param_dm_cos1yr].paramSet[0]==1){
			    dmDot += ((MNStruct *)globalcontext)->pulse->param[param_dm_cos1yr].val[0]*cos(2*M_PI*yrs);
			}
			if(l==0){printf("DMQ: %i %g \n", k, dmDot);}
			DMSignal[k] += (dmDot)*weight;
			DMt += ((MNStruct *)globalcontext)->numChanPerInt[k];
		}

		double *SignalVec;
		double FreqDet = 0;
		double FreqLike = 0;
		int startpos = 0;
		if(((MNStruct *)globalcontext)->incDM==5){

			int FitDMCoeff = 2*((MNStruct *)globalcontext)->numFitDMCoeff;
			double *SignalCoeff = new double[FitDMCoeff];
			for(int i = 0; i < FitDMCoeff; i++){
				SignalCoeff[startpos + i] = Cube[pcount];
				pcount++;
			}

			double Tspan = ((MNStruct *)globalcontext)->Tspan;
			double f1yr = 1.0/3.16e7;

			double DMamp=Cube[pcount];
			pcount++;
			double DMindex=Cube[pcount];
			pcount++;
			
			DMamp=pow(10.0, DMamp);

			for (int i=0; i< FitDMCoeff/2; i++){
				
				double freq = ((double)(i+1.0))/Tspan;
				
				double rho = (DMamp*DMamp)*pow(f1yr,(-3)) * pow(freq*365.25,(-DMindex))/(Tspan*24*60*60);
				SignalCoeff[i] = SignalCoeff[i]*sqrt(rho);
				SignalCoeff[i+FitDMCoeff/2] = SignalCoeff[i+FitDMCoeff/2]*sqrt(rho);  
				FreqDet += 2*log(rho);	
				FreqLike += SignalCoeff[i]*SignalCoeff[i]/rho + SignalCoeff[i+FitDMCoeff/2]*SignalCoeff[i+FitDMCoeff/2]/rho;
			}
			double *FMatrix = new double[FitDMCoeff*((MNStruct *)globalcontext)->numProfileEpochs];
			for(int i=0;i< FitDMCoeff/2;i++){
				int DMt = 0;
				for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){

					double time=(double)((MNStruct *)globalcontext)->pulse->obsn[DMt].bat;
		
					FMatrix[k + (i+startpos)*((MNStruct *)globalcontext)->numProfileEpochs]=cos(2*M_PI*(double(i+1)/Tspan)*time);
					FMatrix[k + (i+FitDMCoeff/2+startpos)*((MNStruct *)globalcontext)->numProfileEpochs] = sin(2*M_PI*(double(i+1)/Tspan)*time);
					DMt += ((MNStruct *)globalcontext)->numChanPerInt[k];

				}
			}

			SignalVec = new double[((MNStruct *)globalcontext)->numProfileEpochs];
			vector_dgemv(FMatrix,SignalCoeff,SignalVec,((MNStruct *)globalcontext)->numProfileEpochs, FitDMCoeff,'N');
			startpos=FitDMCoeff;
			
			 for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){
				DMSignal[k] += (SignalVec[k])*weight;//(SignalVec[k]+dmDot)*weight;
			}
			delete[] FMatrix;	
			delete[] SignalCoeff;	
			delete[] SignalVec;

		}
	

		if(((MNStruct *)globalcontext)->incDM==6){

			int FitDMCoeff = 2*((MNStruct *)globalcontext)->numFitDMCoeff;
			double *SignalCoeff = new double[FitDMCoeff];
			for(int i = 0; i < FitDMCoeff; i++){
				SignalCoeff[startpos + i] = Cube[pcount];
				//printf("coeffs %i %g \n", i, SignalCoeff[i]);
				pcount++;
			}
				
			

			double Tspan = ((MNStruct *)globalcontext)->Tspan;
			double f1yr = 1.0/3.16e7;



			for (int i=0; i< FitDMCoeff/2; i++){
		
				double DMamp=pow(10.0, Cube[pcount]);
				pcount++;
				
				double freq = ((double)(i+1.0))/Tspan;
				
				double rho = (DMamp*DMamp);
				SignalCoeff[i] = SignalCoeff[i]*sqrt(rho);
				SignalCoeff[i+FitDMCoeff/2] = SignalCoeff[i+FitDMCoeff/2]*sqrt(rho);  
				FreqDet += 2*log(rho);	
				FreqLike += SignalCoeff[i]*SignalCoeff[i]/rho + SignalCoeff[i+FitDMCoeff/2]*SignalCoeff[i+FitDMCoeff/2]/rho;
			}
			double *FMatrix = new double[FitDMCoeff*((MNStruct *)globalcontext)->numProfileEpochs];
			for(int i=0;i< FitDMCoeff/2;i++){
				int DMt = 0;
				for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){

					double time=(double)((MNStruct *)globalcontext)->pulse->obsn[DMt].bat;
		
					FMatrix[k + (i+startpos)*((MNStruct *)globalcontext)->numProfileEpochs]=cos(2*M_PI*(double(i+1)/Tspan)*time);
					FMatrix[k + (i+FitDMCoeff/2+startpos)*((MNStruct *)globalcontext)->numProfileEpochs] = sin(2*M_PI*(double(i+1)/Tspan)*time);
					DMt += ((MNStruct *)globalcontext)->numChanPerInt[k];

				}
			}

			SignalVec = new double[((MNStruct *)globalcontext)->numProfileEpochs];
			vector_dgemv(FMatrix,SignalCoeff,SignalVec,((MNStruct *)globalcontext)->numProfileEpochs, FitDMCoeff,'N');
			startpos=FitDMCoeff;
			
			 for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){
				DMSignal[k] += (SignalVec[k])*weight;//(SignalVec[k]+dmDot)*weight;
			}
			delete[] FMatrix;	
			delete[] SignalCoeff;	
			delete[] SignalVec;

		}
	
        }       

 
		
	 for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){
		DMSignal[k] = DMSignal[k]/weightsum;
	}

	summaryfile.close();



	printf("Getting Errors with %i lines \n", number_of_lines);
	summaryfile.open(fname.c_str());	
	weightsum=0;
        for(int l=0;l<number_of_lines;l++){

		if(l%1000==0){printf("Line: %i \n", l);}

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);
                double weight = paramlist[0];
		weightsum+=weight;

		 for(int i = 0; i < ndim; i++){
		         Cube[i] = paramlist[i+2];
		 }


		int dotime = 0;
		struct timeval tval_before, tval_after, tval_resultone, tval_resulttwo;

		int debug = 0;

		if(debug == 1)printf("In like \n");
		long double LDparams[ndim];
		int pcount;




/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


		int fitcount=0;
		for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){

			LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
			fitcount++;

		}	
		
		
		pcount=0;
		long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
		pcount++;
		for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
			((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];
			pcount++;
		}
		for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
			((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
			pcount++;
		}

		long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
		for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

		   JumpVec[p] = 0;
		   for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
			for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
				if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){
					JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
				}
			 }
		   }

		}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form SATS///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


		for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
			((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
		}

		///for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
		//	((MNStruct *)globalcontext)->pulse->jumpVal[k]= 0;
		//}
	       for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		      ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
		}

		delete[] JumpVec;


//		fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);     
		//formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);     
		



		
		if(debug == 1)printf("Phase: %g \n", (double)phase);
		if(debug == 1)printf("Formed Residuals \n");

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////White noise/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


		  
		if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		}
		else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
			pcount++;
			
		}
		else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
			for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
				pcount++;

			}
		}	
		  
		if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		}
		else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
			pcount++;
		}
		else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
			for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
				pcount++;
			}
		}


		if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		}
		else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
			pcount++;
		}
		else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
			for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
				pcount++;
			}
		}	  
		  

		if(((MNStruct *)globalcontext)->incHighFreqStoc == 1){
			pcount++;
		}
		if(((MNStruct *)globalcontext)->incHighFreqStoc == 2){
			pcount++;
			pcount++;
		}

		if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
			pcount++;
		}	
		
		if(((MNStruct *)globalcontext)->incWidthJitter > 0){
			pcount++;
		}



		if(((MNStruct *)globalcontext)->incProfileNoise == 1){

			pcount++;
			pcount++;
		}

		if(((MNStruct *)globalcontext)->incProfileNoise == 2){

			for(int q = 0; q < ((MNStruct *)globalcontext)->ProfileNoiseCoeff; q++){
				pcount++;
			}
		}

		if(((MNStruct *)globalcontext)->incProfileNoise == 3){

			for(int q = 0; q < ((MNStruct *)globalcontext)->ProfileNoiseCoeff; q++){
				pcount++;
			}
		}

		 double *SignalVec;
		 double *NonPLSignalVec = new double[((MNStruct *)globalcontext)->numProfileEpochs]();
		 int DMt = 0;
		 for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){
			double dmDot = 0;
			longdouble yrs = (((MNStruct *)globalcontext)->pulse->obsn[DMt].bat - ((MNStruct *)globalcontext)->pulse->param[param_dmepoch].val[0])/365.25;
			longdouble arg = 1.0;

			for (int d=0;d<9;d++){
			    if(d>0){
				arg *= yrs;
			    }
			    if (((MNStruct *)globalcontext)->pulse->param[param_dm].paramSet[d]==1){
				if(d>0){
				    dmDot+=(double)(((MNStruct *)globalcontext)->pulse->param[param_dm].val[d]*arg);
				}
			    }
			}

			if (((MNStruct *)globalcontext)->pulse->param[param_dm_sin1yr].paramSet[0]==1){
			    dmDot += ((MNStruct *)globalcontext)->pulse->param[param_dm_sin1yr].val[0]*sin(2*M_PI*yrs);
			}
			if (((MNStruct *)globalcontext)->pulse->param[param_dm_cos1yr].paramSet[0]==1){
			    dmDot += ((MNStruct *)globalcontext)->pulse->param[param_dm_cos1yr].val[0]*cos(2*M_PI*yrs);
			}

			NonPLSignalVec[k] += (dmDot);
			DMt += ((MNStruct *)globalcontext)->numChanPerInt[k];
		}


		
		double FreqDet = 0;
		double FreqLike = 0;
		int startpos = 0;
		if(((MNStruct *)globalcontext)->incDM==5){

			int FitDMCoeff = 2*((MNStruct *)globalcontext)->numFitDMCoeff;
			double *SignalCoeff = new double[FitDMCoeff];
			for(int i = 0; i < FitDMCoeff; i++){
				SignalCoeff[startpos + i] = Cube[pcount];
				pcount++;
			}
				
			

			double Tspan = ((MNStruct *)globalcontext)->Tspan;
			double f1yr = 1.0/3.16e7;


			double DMamp=Cube[pcount];
			pcount++;
			double DMindex=Cube[pcount];
			pcount++;

			
			DMamp=pow(10.0, DMamp);



			for (int i=0; i< FitDMCoeff/2; i++){
				
				double freq = ((double)(i+1.0))/Tspan;
				
				double rho = (DMamp*DMamp)*pow(f1yr,(-3)) * pow(freq*365.25,(-DMindex))/(Tspan*24*60*60);
				SignalCoeff[i] = SignalCoeff[i]*sqrt(rho);
				SignalCoeff[i+FitDMCoeff/2] = SignalCoeff[i+FitDMCoeff/2]*sqrt(rho);  



			}
			//double *TempSignalVec   = new double[((MNStruct *)globalcontext)->numProfileEpochs]();
			double *FMatrix = new double[FitDMCoeff*((MNStruct *)globalcontext)->numProfileEpochs];
			for(int i=0;i< FitDMCoeff/2;i++){
				int DMt = 0;
				for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){
					double time=(double)((MNStruct *)globalcontext)->pulse->obsn[DMt].bat;

					//TempSignalVec[k] += SignalCoeff[i]*cos(2*M_PI*(double(i+1)/Tspan)*time) + SignalCoeff[i+FitDMCoeff/2]*sin(2*M_PI*(double(i+1)/Tspan)*time);
					

					if(l==0 && k==0){printf("Freqs: %i %g %g %g\n", i, Tspan, time, (double(i+1)/Tspan));}
					FMatrix[k + (i+startpos)*((MNStruct *)globalcontext)->numProfileEpochs]=cos(2*M_PI*(double(i+1)/Tspan)*time);
					FMatrix[k + (i+FitDMCoeff/2+startpos)*((MNStruct *)globalcontext)->numProfileEpochs] = sin(2*M_PI*(double(i+1)/Tspan)*time);
					//printf("FM: %i %i %g %g \n", i, k, FMatrix[k + (i+startpos)*((MNStruct *)globalcontext)->numProfileEpochs], Tspan);
					DMt += ((MNStruct *)globalcontext)->numChanPerInt[k];

				}
			}

			SignalVec = new double[((MNStruct *)globalcontext)->numProfileEpochs];
			vector_dgemv(FMatrix,SignalCoeff,SignalVec,((MNStruct *)globalcontext)->numProfileEpochs, FitDMCoeff,'N');
			startpos=FitDMCoeff;
			 int DMt = 0;	
			 for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){
				NonPLSignalVec[k] += SignalVec[k];
				DMt += ((MNStruct *)globalcontext)->numChanPerInt[k];
			}





			delete[] FMatrix;	
			delete[] SignalCoeff;	
			delete[] SignalVec;
			//delete[] TempSignalVec;

		}

		if(((MNStruct *)globalcontext)->incDM==6){

			int FitDMCoeff = 2*((MNStruct *)globalcontext)->numFitDMCoeff;
			double *SignalCoeff = new double[FitDMCoeff];
			for(int i = 0; i < FitDMCoeff; i++){
				SignalCoeff[startpos + i] = Cube[pcount];
				//printf("coeffs %i %g \n", i, SignalCoeff[i]);
				pcount++;
			}
				
			

			double Tspan = ((MNStruct *)globalcontext)->Tspan;
			double f1yr = 1.0/3.16e7;



			for (int i=0; i< FitDMCoeff/2; i++){
		
				double DMamp=pow(10.0, Cube[pcount]);
				pcount++;
				
				double freq = ((double)(i+1.0))/Tspan;
				
				double rho = (DMamp*DMamp);
				SignalCoeff[i] = SignalCoeff[i]*sqrt(rho);
				SignalCoeff[i+FitDMCoeff/2] = SignalCoeff[i+FitDMCoeff/2]*sqrt(rho);  
				FreqDet += 2*log(rho);	
				FreqLike += SignalCoeff[i]*SignalCoeff[i]/rho + SignalCoeff[i+FitDMCoeff/2]*SignalCoeff[i+FitDMCoeff/2]/rho;
			}
			double *FMatrix = new double[FitDMCoeff*((MNStruct *)globalcontext)->numProfileEpochs];
			for(int i=0;i< FitDMCoeff/2;i++){
				int DMt = 0;
				for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){

					double time=(double)((MNStruct *)globalcontext)->pulse->obsn[DMt].bat;
		
					FMatrix[k + (i+startpos)*((MNStruct *)globalcontext)->numProfileEpochs]=cos(2*M_PI*(double(i+1)/Tspan)*time);
					FMatrix[k + (i+FitDMCoeff/2+startpos)*((MNStruct *)globalcontext)->numProfileEpochs] = sin(2*M_PI*(double(i+1)/Tspan)*time);
					DMt += ((MNStruct *)globalcontext)->numChanPerInt[k];

				}
			}

			SignalVec = new double[((MNStruct *)globalcontext)->numProfileEpochs];
			vector_dgemv(FMatrix,SignalCoeff,SignalVec,((MNStruct *)globalcontext)->numProfileEpochs, FitDMCoeff,'N');
			startpos=FitDMCoeff;
			 
			int DMt = 0;	
			 for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){
				NonPLSignalVec[k] += SignalVec[k];
				DMt += ((MNStruct *)globalcontext)->numChanPerInt[k];
			}
			delete[] FMatrix;	
			delete[] SignalCoeff;	
			delete[] SignalVec;

		}
		 for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){
			DMSignalErr[k] += (NonPLSignalVec[k] - DMSignal[k])*(NonPLSignalVec[k] - DMSignal[k])*weight;
		}


		
		delete[] NonPLSignalVec;
        }       

	int DMt = 0;
	for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){
		DMSignalErr[k] = sqrt(DMSignalErr[k]/weightsum);
		printf("DM: %i %.10g %g %g \n", k, (double) ((MNStruct *)globalcontext)->pulse->obsn[DMt].bat, DMSignal[k], DMSignalErr[k]);
		DMt += ((MNStruct *)globalcontext)->numChanPerInt[k];
	}

//	summaryfile.close();


}

/*
void  WriteProfileDomainLike2(std::string longname, int &ndim){


	int writeprofiles=1;

	int profiledimstart=0;
        double *Cube = new double[ndim];
        int number_of_lines = 0;

        std::ifstream checkfile;
	std::string checkname;
	if(((MNStruct *)globalcontext)->sampler == 0){
	        checkname = longname+"phys_live.points";
	}
        if(((MNStruct *)globalcontext)->sampler == 1){
                checkname = longname+"_phys_live.txt";
        }
	//printf("%i %s \n", ((MNStruct *)globalcontext)->sampler, checkname.c_str());
        checkfile.open(checkname.c_str());
        std::string line;
        while (getline(checkfile, line))
                ++number_of_lines;

        checkfile.close();

        std::ifstream summaryfile;
	std::string fname;
	if(((MNStruct *)globalcontext)->sampler == 0){
		fname = longname+"phys_live.points";
	}
	if(((MNStruct *)globalcontext)->sampler == 1){
	        fname = longname+"_phys_live.txt";
	}

        summaryfile.open(fname.c_str());



        printf("Getting ML \n");
        double maxlike = -1.0*pow(10.0,10);
        for(int i=0;i<number_of_lines;i++){

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);
                double like = paramlist[ndim];

                if(like > maxlike){
                        maxlike = like;
                         for(int i = 0; i < ndim; i++){
                                 Cube[i] = paramlist[i];
                         }
                }

        }
        summaryfile.close();
	printf("ML val: %.10g \n", maxlike);
//	for(int i = 5; i < ndim; i++){
//		printf("ShapePriors[%i][0] =  %g \n", i-4, ((MNStruct *)globalcontext)->MeanProfileShape[i-4] + Cube[i]);
//		printf("ShapePriors[%i][1] =  %g \n", i-4, ((MNStruct *)globalcontext)->MeanProfileShape[i-4] + Cube[i]);
//	}
	int debug = 0;

	if(debug == 1)printf("In like \n");

	double *EFAC;
	double *EQUAD;



        long double LDparams[ndim];
        int pcount;

	double uniformpriorterm = 0;



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){

		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

	}	
	
	
	pcount=0;
	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
           	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
                	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){
                        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
			}
                 }
           }

	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form SATS///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}


        for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
                ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }

	delete[] JumpVec;

	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);   



	
	if(debug == 1)printf("Phase: %g \n", (double)phase);
	if(debug == 1)printf("Formed Residuals \n");

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////White noise/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=1;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		for(int j =0; j < numcoeff[i]; j=j+2){
			modelflux+=sqrt(sqrt(M_PI))*sqrt(beta)*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Binomial[j])*shapecoeff[cpos+j];
		}
		cpos+= numcoeff[i];
	}

	double OPLevel = ((MNStruct *)globalcontext)->offPulseLevel;

	double lnew = 0;
	int badshape = 0;

//	int minoffpulse=750;
//	int maxoffpulse=1250;
       // int minoffpulse=200;
       // int maxoffpulse=800;
	double maxchisq = 0;	
	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
		if(debug == 1)printf("In toa %i \n", t);
		int nTOA = t;

		double detN  = 0;
		double Chisq  = 0;
		double logMargindet = 0;
		double Marginlike = 0;	 
		double JitterPrior = 0;	   

		double profilelike=0;

		long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
		long double FoldingPeriodDays = FoldingPeriod/SECDAY;
		int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
		double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
		double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
		long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

		double *Betatimes = new double[Nbins];
		double **AllHermiteBasis =  new double*[Nbins];
		double **JitterBasis  =  new double*[Nbins];
		double **Hermitepoly =  new double*[Nbins];

		for(int i =0;i<Nbins;i++){AllHermiteBasis[i]=new double[maxshapecoeff];for(int j =0;j<maxshapecoeff;j++){AllHermiteBasis[i][j]=0;}}
		for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[totshapecoeff];for(int j =0;j<totshapecoeff;j++){Hermitepoly[i][j]=0;}}
		for(int i =0;i<Nbins;i++){JitterBasis[i]=new double[totshapecoeff];for(int j =0;j<totshapecoeff;j++){JitterBasis[i][j]=0;}}
	
	

		long double binpos = ModelBats[nTOA];


		if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

		long double minpos = binpos - FoldingPeriodDays/2;
		if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];

		int cpos=0;
		int jpos=0;
	   	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
			for(int j =0; j < Nbins; j++){
				long double timediff = 0;
				long double bintime = ProfileBats[t][j] + CompSeps[i]/SECDAY;
				if(bintime  >= minpos && bintime <= maxpos){
				    timediff = bintime - binpos;
				}
				else if(bintime < minpos){
				    timediff = FoldingPeriodDays+bintime - binpos;
				}
				else if(bintime > maxpos){
				    timediff = bintime - FoldingPeriodDays - binpos;
				}

				timediff=timediff*SECDAY;

				Betatimes[j]=timediff/beta;
				TNothplMC(numcoeff[i]+1,Betatimes[j],AllHermiteBasis[j], jpos);

				for(int k =0; k < numcoeff[i]+1; k++){
					double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));
					AllHermiteBasis[j][k+jpos]=AllHermiteBasis[j][k+jpos]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);

					if(k<numcoeff[i]){ Hermitepoly[j][k+cpos] = AllHermiteBasis[j][k+jpos];}
				
				}

				JitterBasis[j][0+cpos] = (1.0/sqrt(2.0))*(-1.0*AllHermiteBasis[j][1+jpos]);
				for(int k =1; k < numcoeff[i]; k++){
					JitterBasis[j][k+cpos] = (1.0/sqrt(2.0))*(sqrt(double(k))*AllHermiteBasis[j][k-1+jpos] - sqrt(double(k+1))*AllHermiteBasis[j][k+1+jpos]);
				}	
		        } 
			cpos += numcoeff[i];
			jpos += numcoeff[i]+1;
		}

		double *shapevec  = new double[Nbins];
		double *Jittervec = new double[Nbins];
		double *ProfileModVec = new double[Nbins]();
		double *ProfileJitterModVec = new double[Nbins]();

		double OverallProfileAmp = shapecoeff[0];

		shapecoeff[0] = 1;

		dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,totshapecoeff,'N');
		dgemv(JitterBasis,shapecoeff,Jittervec,Nbins,totshapecoeff,'N');


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////Add in any Profile Changes///////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		for(int i =0; i < totalCoeffForMult; i++){
			ProfileModCoeffs[i]=0;	
		}				

		if(nTOA==0){printf("outputting profile parameters\n");}


		cpos = 0;
		epos = 0;
		fpos = 0;

		for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){

			for(int i =0; i < numProfileFitCoeff[c]; i++){
				ProfileModCoeffs[i+cpos] += ProfileFitCoeffs[i+fpos];

				if(nTOA==0){printf("P %g \n", ((MNStruct *)globalcontext)->MeanProfileShape[i+npos] + ProfileFitCoeffs[i+fpos]);}

			}
			for(int i = numProfileFitCoeff[c]; i < numcoeff[c]; i++){
				if(nTOA==0){printf("P %g \n", ((MNStruct *)globalcontext)->MeanProfileShape[i+npos]);}
			}
		
			if(nTOA==0 && c == 0){printf("B %g \n", ((MNStruct *)globalcontext)->MeanProfileBeta );}

			double reffreq = ((MNStruct *)globalcontext)->EvoRefFreq;
			double freqdiff =  (((MNStruct *)globalcontext)->pulse->obsn[t].freq - reffreq)/1000.0;

			for(int i =0; i < numEvoCoeff[c]; i++){
				ProfileModCoeffs[i+cpos] += EvoCoeffs[i+epos]*freqdiff;
				if(nTOA==0){printf("E %g \n", EvoCoeffs[i+epos]);}
			}
	
			cpos += NumCoeffForMult[c];
			epos += numEvoCoeff[c];
			fpos += numProfileFitCoeff[c];
			npos += numcoeff[c];


		}

		if(nTOA==0){printf("finished outputting profile parameters\n");}

 		double ModModelFlux = modelflux;
		cpos = 0;
		for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
			for(int j =0; j < NumCoeffForMult[i]; j=j+2){
				modelflux+=sqrt(sqrt(M_PI))*sqrt(beta)*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Binomial[j])*ProfileModCoeffs[cpos+j];
			}
			cpos += NumCoeffForMult[i];
		}


		if(totalCoeffForMult > 0){
			dgemv(Hermitepoly, ProfileModCoeffs,ProfileModVec,Nbins,totalCoeffForMult,'N');

			if(((MNStruct *)globalcontext)->numFitEQUAD > 0 || ((MNStruct *)globalcontext)->incDMEQUAD > 0 ){
				dgemv(JitterBasis, ProfileModCoeffs,ProfileJitterModVec,Nbins,totalCoeffForMult,'N');
			}
		}





		double *NDiffVec = new double[Nbins];

		double maxshape=0;


		for(int j =0; j < Nbins; j++){
			shapevec[j] += ProfileModVec[j];
		  	if(shapevec[j] > maxshape){ maxshape = shapevec[j];}

		}


		//maxshape=((MNStruct *)globalcontext)->MaxShapeAmp;

		double noisemean=0;
		int numoffpulse = 0;
		for(int j = 0; j < Nbins; j++){
			if(shapevec[j] < maxshape*OPLevel){
				noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
				numoffpulse += 1;
			}	
		}


		noisemean = noisemean/(numoffpulse);

		double MLSigma = 0;
		int MLSigmaCount = 0;
		double dataflux = 0;
		for(int j = 0; j < Nbins; j++){
			if(shapevec[j] < maxshape*OPLevel){
				double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean;
				MLSigma += res*res; MLSigmaCount += 1;
			}
			else{
				if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean)/MLSigma > 1){
					dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-noisemean)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
				}
			}
		}

		MLSigma = sqrt(MLSigma/MLSigmaCount);
		MLSigma = MLSigma*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];
		double maxamp = dataflux/ModModelFlux;
		if(debug == 1)printf("noise %g, dataflux %g, modelflux %g, maxamp %g \n", MLSigma, dataflux, ModModelFlux, maxamp);


	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


		double **M = new double*[Nbins];
		double **NM = new double*[Nbins];

		int Msize = 2+numshapestoccoeff;
		int Mcounter=0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
			Msize++;
		}


		for(int i =0; i < Nbins; i++){
			Mcounter=0;
			M[i] = new double[Msize];
			NM[i] = new double[Msize];

			M[i][0] = 1;
			M[i][1] = shapevec[i];

			Mcounter = 2;

			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
				M[i][Mcounter] = Jittervec[i];
				Mcounter++;
			}			

			for(int j = 0; j < numshapestoccoeff; j++){
			    M[i][Mcounter+j] = AllHermiteBasis[i][j];
			}
		}


		double **MNM = new double*[Msize];
		for(int i =0; i < Msize; i++){
		    MNM[i] = new double[Msize];
		}

		      
		Chisq = 0;
		detN = 0;

	
 		double highfreqnoise = HighFreqStoc1;
		double flatnoise = HighFreqStoc2;
		
		double *profilenoise = new double[Nbins];

		for(int i =0; i < Nbins; i++){
			Mcounter=2;
			double noise = MLSigma*MLSigma +  pow(highfreqnoise*maxamp*shapevec[i],2);
			if(shapevec[i] > maxshape*OPLevel){
				noise +=  pow(flatnoise*maxamp*maxshape,2);
			}
			detN += log(noise);
			profilenoise[i] = sqrt(noise);

			double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1];

			NDiffVec[i] = datadiff/(noise);
		       

			Chisq += datadiff*datadiff/(noise);
	
			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
				M[i][2] = M[i][2]*dataflux/ModModelFlux/beta;
				Mcounter++;
			}
			
			for(int j = 0; j < numshapestoccoeff; j++){
				M[i][Mcounter+j] = M[i][Mcounter+j]*dataflux;

			}


			for(int j = 0; j < Msize; j++){
				NM[i][j] = M[i][j]/(noise);
			}


		}
		
			
		dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');


		Mcounter=2;
		double JitterDet = 0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
			double profnoise = EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
			profnoise = profnoise*profnoise;
			JitterDet = log(profnoise);
			MNM[2][2] += 1.0/profnoise;
			Mcounter++;
		}


		double StocProfDet = 0;
		for(int j = 0; j < numshapestoccoeff; j++){
			StocProfDet += log(StocProfCoeffs[j]);
			MNM[Mcounter+j][Mcounter+j] +=  1.0/StocProfCoeffs[j];
		}

		double *dNM = new double[Msize];
		double *TempdNM = new double[Msize];
		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
		}


		int info=0;
		double Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);
			    
		logMargindet = Margindet;

		Marginlike = 0;
		for(int i =0; i < Msize; i++){
			Marginlike += TempdNM[i]*dNM[i];
		}


		double finaloffset = TempdNM[0];
		double finalamp = TempdNM[1];
		double finalJitter = 0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0)finalJitter = TempdNM[2];

		double *StocVec = new double[Nbins];
		
		for(int i =0; i < Nbins; i++){
			if(((MNStruct *)globalcontext)->numFitEQUAD > 0)Jittervec[i] = M[i][2];
			if(((MNStruct *)globalcontext)->numFitEQUAD == 0)Jittervec[i] = 0;
		}

		dgemv(M,TempdNM,shapevec,Nbins,Msize,'N');

		

		TempdNM[0] = 0;
		TempdNM[1] = 0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0)TempdNM[2] = 0;
		

		dgemv(M,TempdNM,StocVec,Nbins,Msize,'N');

		double StocSN = 0;

		if(writeprofiles == 1){
			std::ofstream profilefile;
			char toanum[15];
			sprintf(toanum, "%d", nTOA);
			std::string ProfileName =  toanum;
			std::string dname = longname+ProfileName+"-Profile.txt";
	
			profilefile.open(dname.c_str());
			double MLChisq = 0;
			for(int i =0; i < Nbins; i++){
				StocSN += StocVec[i]*StocVec[i]/profilenoise[i]/profilenoise[i];
				MLChisq += pow(((double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1] - shapevec[i])/profilenoise[i], 2);
				profilefile << i << " " << std::setprecision(10) << (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1] << " " << shapevec[i] << " " << profilenoise[i] << " " << finaloffset + finalamp*M[i][1] << " " << finalJitter*Jittervec[i] << " " << StocVec[i] << "\n";
//				if(fabs((double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1] - shapevec[i])/profilenoise[i] > 5)printf("5 sig %i %i \n", t, i);

			}
	    		profilefile.close();
			printf("Stoc SN: %i %g \n", t, StocSN);
		}
		delete[] profilenoise;
		delete[] StocVec;
		
		profilelike = -0.5*(detN + Chisq + logMargindet + StocProfDet + JitterDet - Marginlike);
		lnew += profilelike;

//		if(MLChisq > maxchisq){ printf("Max: %i %g \n", t, MLChisq); maxchisq=MLChisq;}
//		printf("Profile chisq and like: %g %g \n", MLChisq, profilelike);
//		printf("Like: %i %.15g %.15g %.15g %.15g %.15g %.15g %.15g\n", nTOA,lnew, detN, Chisq, logMargindet, Marginlike, StocProfDet , JitterDet);

		delete[] shapevec;
		delete[] Jittervec;
		delete[] ProfileModVec;
		delete[] ProfileJitterModVec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] Betatimes;

		for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] JitterBasis[j];
		    delete[] AllHermiteBasis[j];
		    delete[] M[j];
		    delete[] NM[j];
		}
		delete[] Hermitepoly;
		delete[] AllHermiteBasis;
		delete[] JitterBasis;
		delete[] M;
		delete[] NM;

		for (int j = 0; j < Msize; j++){
		    delete[] MNM[j];
		}
		delete[] MNM;
	
	
	}
	 
	 
	 
	 

	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;
	delete[] numcoeff;
	delete[] CompSeps;
	delete[] NumCoeffForMult;
	lnew += uniformpriorterm;
	printf("End Like: %.10g \n", lnew);

	WriteProfileFreqEvo(longname, ndim,profiledimstart);


}

*/
void  WriteProfileFreqEvo(std::string longname, int &ndim, int profiledimstart){


	int InterpIndex = 0;
        double *Cube = new double[ndim];
        int number_of_lines = 0;

        std::ifstream checkfile;
	std::string checkname;
	if(((MNStruct *)globalcontext)->sampler == 0){
	        checkname = longname+".txt";
	}
        if(((MNStruct *)globalcontext)->sampler == 1){
                checkname = longname+".txt";
        }
	//printf("%i %s \n", ((MNStruct *)globalcontext)->sampler, checkname.c_str());
        checkfile.open(checkname.c_str());
        std::string line;
        while (getline(checkfile, line))
                ++number_of_lines;

        checkfile.close();

        std::ifstream summaryfile;
	std::string fname;
	if(((MNStruct *)globalcontext)->sampler == 0){
		fname = longname+".txt";
	}
	if(((MNStruct *)globalcontext)->sampler == 1){
	        fname = longname+".txt";
	}

        summaryfile.open(fname.c_str());



        printf("Getting Mean with %i lines \n", number_of_lines);

	int numsteps=5;
	int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[0][1];	

	double minfreq = 100000000.0;
	double maxfreq = 0;

	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
		double tfreq = (double)((MNStruct *)globalcontext)->pulse->obsn[t].freq;
		if(tfreq > maxfreq)maxfreq = tfreq;
		if(tfreq < minfreq)minfreq = tfreq;
	}

	printf("Using freqs: min %g max %g \n", minfreq, maxfreq);


	double bw = maxfreq-minfreq;
	double fstep = bw/(numsteps-1);		



	double **EvoProfiles = new double*[numsteps];
	for(int i = 0; i < numsteps; i++){
		EvoProfiles[i] = new double[Nbins];
		for(int j = 0; j < Nbins; j++){
			EvoProfiles[i][j] = 0;
		}
	}

	double **EvoErrProfiles = new double*[numsteps];
	for(int i = 0; i < numsteps; i++){
		EvoErrProfiles[i] = new double[Nbins];
		for(int j = 0; j < Nbins; j++){
			EvoErrProfiles[i][j] = 0;
		}
	}

	double weightsum=0;
        for(int i=0;i<number_of_lines;i++){

		if(i%1000==0){printf("Line: %i \n", i);}

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);
                double weight = paramlist[0];
		weightsum+=weight;

		 for(int i = 0; i < ndim; i++){
		         Cube[i] = paramlist[i+2];
		 }
                





/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

		int cpos = 0;
		int epos = 0;
		int fpos = 0;
		int npos = 0;

		int pcount = profiledimstart;

		int maxshapecoeff = 0;
		int totshapecoeff = ((MNStruct *)globalcontext)->totshapecoeff; 

		int *numcoeff= new int[((MNStruct *)globalcontext)->numProfComponents];
		for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
			numcoeff[i] =  ((MNStruct *)globalcontext)->numshapecoeff[i];
		}

		int *numshapestoccoeff = ((MNStruct *)globalcontext)->numshapestoccoeff;
		int totalshapestoccoeff =  ((MNStruct *)globalcontext)->totalshapestoccoeff;

		int *numEvoCoeff = ((MNStruct *)globalcontext)->numEvoCoeff;
		int totalEvoCoeff = ((MNStruct *)globalcontext)->totalEvoCoeff;

		int *numFitEvoCoeff = ((MNStruct *)globalcontext)->numEvoFitCoeff;
		int totalFitEvoCoeff  = ((MNStruct *)globalcontext)->totalEvoFitCoeff;

		int *numProfileFitCoeff = ((MNStruct *)globalcontext)->numProfileFitCoeff;
		int totalProfileFitCoeff = ((MNStruct *)globalcontext)->totalProfileFitCoeff;


		int totalCoeffForMult = 0;
		int *NumCoeffForMult = new int[((MNStruct *)globalcontext)->numProfComponents];
		for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
			NumCoeffForMult[i] = 0;
			if(numEvoCoeff[i] > NumCoeffForMult[i]){NumCoeffForMult[i]=numEvoCoeff[i];}
			if(numProfileFitCoeff[i] > NumCoeffForMult[i]){NumCoeffForMult[i]=numProfileFitCoeff[i];}
			totalCoeffForMult += NumCoeffForMult[i];
		}

		double shapecoeff[totshapecoeff];
		double StocProfCoeffs[totalshapestoccoeff];
		double EvoCoeffs[totalEvoCoeff];
		double ProfileFitCoeffs[totalProfileFitCoeff];
		double ProfileModCoeffs[totalCoeffForMult];


		for(int i =0; i < totshapecoeff; i++){
			shapecoeff[i]=((MNStruct *)globalcontext)->MeanProfileShape[i];
		}
		double beta = ((MNStruct *)globalcontext)->MeanProfileBeta*((MNStruct *)globalcontext)->ReferencePeriod;

		for(int i =0; i < totalEvoCoeff; i++){
			EvoCoeffs[i]=((MNStruct *)globalcontext)->MeanProfileEvo[i];
//			printf("EC %i %g \n", i, EvoCoeffs[i]);
		}

		profiledimstart=pcount;
		for(int i =0; i < totalshapestoccoeff; i++){
			StocProfCoeffs[i]= pow(10.0, 2*Cube[pcount]);
			pcount++;
		}

		for(int i =0; i < totalProfileFitCoeff; i++){
			ProfileFitCoeffs[i]= Cube[pcount];
			pcount++;
		}
		double LinearProfileWidth=0;
		if(((MNStruct *)globalcontext)->FitLinearProfileWidth == 1){
			LinearProfileWidth = Cube[pcount];
			pcount++;
		}


		for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
			for(int i =0; i < numFitEvoCoeff[c]; i++){
				EvoCoeffs[i+cpos] += Cube[pcount];
//				printf("MEC %i %g \n", i, EvoCoeffs[i]);
				pcount++;
			}
			cpos += numEvoCoeff[c];
		}

		double EvoProfileWidth=0;
		if(((MNStruct *)globalcontext)->incProfileEvo == 2){
			EvoProfileWidth = Cube[pcount];
			pcount++;
		}

	/*
		if(totshapecoeff+1>=numshapestoccoeff+1){
			maxshapecoeff=totshapecoeff+1;
		}
		if(numshapestoccoeff+1 > totshapecoeff+1){
			maxshapecoeff=numshapestoccoeff+1;
		}
	*/
		maxshapecoeff = totshapecoeff + ((MNStruct *)globalcontext)->numProfComponents;

		long double *CompSeps = new long double[((MNStruct *)globalcontext)->numProfComponents];
		CompSeps[0] = 0;
		for(int i = 1; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		        CompSeps[i] = 0.336277178806697163*((MNStruct *)globalcontext)->ReferencePeriod;
		}


		for(int s = 0; s < numsteps; s++){

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


		

			double reffreq = ((MNStruct *)globalcontext)->EvoRefFreq;
			double freq = minfreq + s*fstep;
			double freqdiff =  (freq - reffreq)/1000.0;



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////Add in any Profile Changes///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



			double *shapevec  = new double[Nbins];
			double *ProfileModVec  = new double[Nbins];

			for(int i =0; i < totalCoeffForMult; i++){
				ProfileModCoeffs[i]=0;	
			}	

			
			cpos = 0;
			epos = 0;
			fpos = 0;
			npos = 0;

			for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){

							

				for(int i =0; i < numProfileFitCoeff[c]; i++){
					ProfileModCoeffs[i+cpos] += ProfileFitCoeffs[i+fpos];
				}


				for(int i =0; i < numEvoCoeff[c]; i++){
					//if(t==0){printf("fill evo coeff %i %i %g %g \n", NumCoeffForMult, i,  ProfileFitCoeffs[i], EvoCoeffs[i]*freqdiff);}
					ProfileModCoeffs[i+cpos] += EvoCoeffs[i+epos]*freqdiff;
		
				}

				cpos += NumCoeffForMult[c];
				epos += numEvoCoeff[c];
				fpos += numProfileFitCoeff[c];


			}

			if(totalCoeffForMult > 0){
				vector_dgemv(((MNStruct *)globalcontext)->InterpolatedShapeletsVec[InterpIndex], ProfileModCoeffs,ProfileModVec,Nbins,totalCoeffForMult,'N');
			}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////Fill Arrays with interpolated state//////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	
	
	
			for(int j =0; j < Nbins; j++){


				double widthTerm = ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpIndex][j]*LinearProfileWidth; 
				double evoWidthTerm = ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpIndex][j]*EvoProfileWidth*freqdiff;

				EvoProfiles[s][j] += (((MNStruct *)globalcontext)->InterpolatedMeanProfile[InterpIndex][j] + widthTerm + ProfileModVec[j] + evoWidthTerm)*weight;
				//printf("Evo: %g %i %g \n", freq, j, shapevec[j]);
			}

			delete[] shapevec;
			delete[] ProfileModVec;
		}

		delete[] numcoeff;

	}

	for(int s = 0; s < numsteps; s++){
		for(int j =0; j < Nbins; j++){
			EvoProfiles[s][j] = EvoProfiles[s][j]/weightsum;
		}
	}

	summaryfile.close();

	printf("Getting Errors with %i lines \n", number_of_lines);
	summaryfile.open(fname.c_str());	
	weightsum=0;
        for(int i=0;i<number_of_lines;i++){

		if(i%1000==0){printf("Line: %i \n", i);}

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);
                double weight = paramlist[0];
		weightsum+=weight;

		 for(int i = 0; i < ndim; i++){
		         Cube[i] = paramlist[i+2];
		 }
                





/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

		int cpos = 0;
		int epos = 0;
		int fpos = 0;
		int npos = 0;


		int pcount = profiledimstart;

		int maxshapecoeff = 0;
		int totshapecoeff = ((MNStruct *)globalcontext)->totshapecoeff; 

		int *numcoeff= new int[((MNStruct *)globalcontext)->numProfComponents];
		for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
			numcoeff[i] =  ((MNStruct *)globalcontext)->numshapecoeff[i];
		}

		int *numshapestoccoeff = ((MNStruct *)globalcontext)->numshapestoccoeff;
		int totalshapestoccoeff = ((MNStruct *)globalcontext)->totalshapestoccoeff;

		int *numEvoCoeff = ((MNStruct *)globalcontext)->numEvoCoeff;
		int totalEvoCoeff = ((MNStruct *)globalcontext)->totalEvoCoeff;

		int *numFitEvoCoeff = ((MNStruct *)globalcontext)->numEvoFitCoeff;
		int totalFitEvoCoeff  = ((MNStruct *)globalcontext)->totalEvoFitCoeff;

		int *numProfileFitCoeff = ((MNStruct *)globalcontext)->numProfileFitCoeff;
		int totalProfileFitCoeff = ((MNStruct *)globalcontext)->totalProfileFitCoeff;


		int totalCoeffForMult = 0;
		int *NumCoeffForMult = new int[((MNStruct *)globalcontext)->numProfComponents];
		for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
			NumCoeffForMult[i] = 0;
			if(numEvoCoeff[i] > NumCoeffForMult[i]){NumCoeffForMult[i]=numEvoCoeff[i];}
			if(numProfileFitCoeff[i] > NumCoeffForMult[i]){NumCoeffForMult[i]=numProfileFitCoeff[i];}
			totalCoeffForMult += NumCoeffForMult[i];
		}

		double shapecoeff[totshapecoeff];
		double StocProfCoeffs[totalshapestoccoeff];
		double EvoCoeffs[totalEvoCoeff];
		double ProfileFitCoeffs[totalProfileFitCoeff];
		double ProfileModCoeffs[totalCoeffForMult];


		for(int i =0; i < totshapecoeff; i++){
			shapecoeff[i]=((MNStruct *)globalcontext)->MeanProfileShape[i];
		}
		double beta = ((MNStruct *)globalcontext)->MeanProfileBeta*((MNStruct *)globalcontext)->ReferencePeriod;

		for(int i =0; i < totalEvoCoeff; i++){
			EvoCoeffs[i]=((MNStruct *)globalcontext)->MeanProfileEvo[i];
		//	printf("EC %i %g \n", i, EvoCoeffs[i]);
		}

		profiledimstart=pcount;
		for(int i =0; i < totalshapestoccoeff; i++){
			StocProfCoeffs[i]= pow(10.0, 2*Cube[pcount]);
			pcount++;
		}

		for(int i =0; i < totalProfileFitCoeff; i++){
			ProfileFitCoeffs[i]= Cube[pcount];
			pcount++;
		}
		double LinearProfileWidth=0;
		if(((MNStruct *)globalcontext)->FitLinearProfileWidth == 1){
			LinearProfileWidth = Cube[pcount];
			pcount++;
		}


		for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
			for(int i =0; i < numFitEvoCoeff[c]; i++){
				EvoCoeffs[i+cpos] += Cube[pcount];
		//		printf("MEC %i %g \n", i, EvoCoeffs[i]);
				pcount++;
			}
			cpos += numEvoCoeff[c];
		}

		double EvoProfileWidth=0;
		if(((MNStruct *)globalcontext)->incProfileEvo == 2){
			EvoProfileWidth = Cube[pcount];
			pcount++;
		}

	/*
		if(totshapecoeff+1>=numshapestoccoeff+1){
			maxshapecoeff=totshapecoeff+1;
		}
		if(numshapestoccoeff+1 > totshapecoeff+1){
			maxshapecoeff=numshapestoccoeff+1;
		}
	*/
		maxshapecoeff = totshapecoeff + ((MNStruct *)globalcontext)->numProfComponents;

		long double *CompSeps = new long double[((MNStruct *)globalcontext)->numProfComponents];
		CompSeps[0] = 0;
		for(int i = 1; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		        CompSeps[i] = 0.336277178806697163*((MNStruct *)globalcontext)->ReferencePeriod;
		}
		



		for(int s = 0; s < numsteps; s++){

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


		

			double reffreq = ((MNStruct *)globalcontext)->EvoRefFreq;
			double freq = minfreq + s*fstep;
			double freqdiff =  (freq - reffreq)/1000.0;



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////Add in any Profile Changes///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



			double *shapevec  = new double[Nbins];
			double *ProfileModVec  = new double[Nbins];

			for(int i =0; i < totalCoeffForMult; i++){
				ProfileModCoeffs[i]=0;	
			}	

			
			cpos = 0;
			epos = 0;
			fpos = 0;
			npos = 0;

			for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){

							

				for(int i =0; i < numProfileFitCoeff[c]; i++){
					ProfileModCoeffs[i+cpos] += ProfileFitCoeffs[i+fpos];
				}


				for(int i =0; i < numEvoCoeff[c]; i++){
					//if(t==0){printf("fill evo coeff %i %i %g %g \n", NumCoeffForMult, i,  ProfileFitCoeffs[i], EvoCoeffs[i]*freqdiff);}
					ProfileModCoeffs[i+cpos] += EvoCoeffs[i+epos]*freqdiff;
		
				}

				cpos += NumCoeffForMult[c];
				epos += numEvoCoeff[c];
				fpos += numProfileFitCoeff[c];


			}

			if(totalCoeffForMult > 0){
				vector_dgemv(((MNStruct *)globalcontext)->InterpolatedShapeletsVec[InterpIndex], ProfileModCoeffs,ProfileModVec,Nbins,totalCoeffForMult,'N');
			}

			

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////Fill Arrays with interpolated state//////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	
	
	
			for(int j =0; j < Nbins; j++){


				double widthTerm = ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpIndex][j]*LinearProfileWidth; 
				double evoWidthTerm = ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpIndex][j]*EvoProfileWidth*freqdiff;

				double diff = (((MNStruct *)globalcontext)->InterpolatedMeanProfile[InterpIndex][j] + widthTerm + ProfileModVec[j] + evoWidthTerm - EvoProfiles[s][j]);
	
				EvoErrProfiles[s][j] += diff*diff*weight;
				//printf("Evo: %g %i %g \n", freq, j, shapevec[j]);
			}

			delete[] shapevec;
			delete[] ProfileModVec;
		}
		delete[] numcoeff;

	}

	for(int s = 0; s < numsteps; s++){
		for(int j =0; j < Nbins; j++){
			EvoErrProfiles[s][j] = sqrt(EvoErrProfiles[s][j]/weightsum);
		}
	}

	summaryfile.close();

	printf("Evo %g %g %g %g %g \n",minfreq + 0*fstep, minfreq + 1*fstep, minfreq + 2*fstep, minfreq + 3*fstep, minfreq + 4*fstep);
	for(int j =0; j < Nbins; j++){
		printf("Evo %i %g %g %g %g %g %g %g %g %g %g \n", j, EvoProfiles[0][j], EvoProfiles[1][j], EvoProfiles[2][j], EvoProfiles[3][j], EvoProfiles[4][j], EvoErrProfiles[0][j], EvoErrProfiles[1][j], EvoErrProfiles[2][j], EvoErrProfiles[3][j], EvoErrProfiles[4][j]);

	//	printf("Evo %.10g \n", EvoProfiles[0][j]);
	}



}



void Template2DProfLikeMNWrap(double *Cube, int &ndim, int &npars, double &lnew, void *context){



        for(int p=0;p<ndim;p++){

                Cube[p]=(((MNStruct *)globalcontext)->PriorsArray[p+ndim]-((MNStruct *)globalcontext)->PriorsArray[p])*Cube[p]+((MNStruct *)globalcontext)->PriorsArray[p];
        }


	double *DerivedParams = new double[npars];

	double result = Template2DProfLike(ndim, Cube, npars, DerivedParams, context);


	delete[] DerivedParams;

	lnew = result;

}

double  Template2DProfLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){

	//printf("In like \n");


	int debug = 0;
	double TemplateFlux=0;
	double FakeRMS = 0;
	double TargetSN=0;
        long double LDparams;
        int pcount;

	double uniformpriorterm = 0;


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	LDparams=Cube[0]*(((MNStruct *)globalcontext)->LDpriors[0][1]) + (((MNStruct *)globalcontext)->LDpriors[0][0]);
	
	
	pcount=0;
	long double phase = LDparams*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

		   
	int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[0][1];

	long double *ProfileBats = new long double[Nbin];

	long double FoldingPeriod = ((MNStruct *)globalcontext)->ReferencePeriod;
	long double FoldingPeriodDays = FoldingPeriod/SECDAY;

	long double pulsesamplerate = FoldingPeriod/Nbin/SECDAY;

	for(int b =0; b < Nbin; b++){
		 ProfileBats[b] = pulsesamplerate*b;
	}

	TemplateFlux=0;


	int totalProfCoeff = 0;
	int *numcoeff= new int[((MNStruct *)globalcontext)->numProfComponents];

	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		numcoeff[i] =  ((MNStruct *)globalcontext)->numshapecoeff[i];
		totalProfCoeff += numcoeff[i];
	}

	double oldbeta=0.00373642;
	//double beta = oldbeta*((MNStruct *)globalcontext)->ReferencePeriod;
	double beta = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;


	//double Pscale = pow(10.0, 2*Cube[pcount]);
	double PDet = 0;
	pcount++;

	long double *CompSeps = new long double[((MNStruct *)globalcontext)->numProfComponents];
	CompSeps[0] = 0;
	for(int i = 1; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		CompSeps[i] = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;
		pcount++;
		//printf("CompSep: %g \n", Cube[pcount-1]);
	}

	if(debug ==1){printf("Params: %g %g \n", Cube[0], Cube[1]);}

	

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	double lnew = 0;
	int badshape = 0;


	


	int nTOA = 0;


	double logMargindet = 0;
	double Marginlike = 0;	 

	double profilelike=0;


	int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
	double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
	long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

	double *Betatimes = new double[Nbins];
	double **Hermitepoly =  new double*[Nbins];
	//double **JitterBasis =  new double*[Nbins];
	for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[totalProfCoeff];for(int j =0;j<totalProfCoeff;j++){Hermitepoly[i][j]=0;}}
	//for(int i =0;i<Nbins;i++){JitterBasis[i]=new double[totalProfCoeff];for(int j =0;j<totalProfCoeff;j++){JitterBasis[i][j]=0;}}


	long double binpos = phase;




	long double minpos = binpos - FoldingPeriodDays/2;
	if(minpos < ProfileBats[0])minpos=ProfileBats[0];
	long double maxpos = binpos + FoldingPeriodDays/2;
	if(maxpos> ProfileBats[Nbins-1])maxpos =ProfileBats[Nbins-1];


    	int cpos = 0;
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		for(int j =0; j < Nbins; j++){
			long double timediff = 0;
			long double bintime = ProfileBats[j]+CompSeps[i]/SECDAY;
			if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
			}
			else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
			}
			else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
			}

			timediff=timediff*SECDAY;
		

			Betatimes[j]=(timediff)/beta;
			TNothplMC(numcoeff[i],Betatimes[j],Hermitepoly[j], cpos);

			for(int k =0; k < numcoeff[i]; k++){
				//if(k==0)printf("HP %i %i %g %g \n", i, j, Betatimes[j],Hermitepoly[j][cpos+k]*exp(-0.5*Betatimes[j]*Betatimes[j]));
				double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));//*((MNStruct *)globalcontext)->Factorials[k]);
				Hermitepoly[j][cpos+k]=Hermitepoly[j][cpos+k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);

			}

			//JitterBasis[j][0+cpos] = (1.0/sqrt(2.0))*(-1.0*Hermitepoly[j][1+jpos]);
			//for(int k =1; k < numcoeff[i]; k++){
			//	JitterBasis[j][k+cpos] = (1.0/sqrt(2.0))*(sqrt(double(k))*AllHermiteBasis[j][k-1+jpos] - sqrt(double(k+1))*AllHermiteBasis[j][k+1+jpos]);
			//}
				
		}
		cpos += numcoeff[i];
   	 }

	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


	double **M = new double*[Nbins];

	int Msize = totalProfCoeff+1;
	for(int i =0; i < Nbins; i++){
		M[i] = new double[Msize];

		M[i][0] = 1;


		for(int j = 0; j < totalProfCoeff; j++){
			M[i][j+1] = Hermitepoly[i][j];
			//if(j==0)printf("%i %i %g \n", i, j, M[i][j+1]);
		}
	  
	}


	double **MNM = new double*[Msize];
	for(int i =0; i < Msize; i++){
	    MNM[i] = new double[Msize];
	}

	double **TempMNM = new double*[Msize];
	for(int i =0; i < Msize; i++){
	    TempMNM[i] = new double[Msize];
	}

	dgemm(M, M , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');

	for(int j = 1; j < Msize; j++){
		//MNM[j][j] += 1.0/Pscale;//pow(10.0,-14);
		MNM[j][j] += pow(10.0,-14);
		//PDet+=log(Pscale);
	}

	double Chisq=0;
	double detN = 0;

	for(int fc=0; fc < ((MNStruct *)globalcontext)->numTempFreqs; fc++){
	

		double *dNM = new double[Msize]();
		double *TempdNM = new double[Msize]();
		double *NDiffVec = new double[Nbins]();
		double *shapevec = new double[Nbins]();
		//double *Jittervec = new double[Nbins]();

		for(int j = 0; j < Nbins; j++){
			NDiffVec[j] = (double)((MNStruct *)globalcontext)->TemplateChans[fc*Nbins + j];
		}
		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
			for(int j =0; j < Msize; j++){
				TempMNM[i][j] = MNM[i][j];
			}
		}


		int info=0;
		double Margindet = 0;
		dpotrfInfo(TempMNM, Msize, Margindet, info);
		dpotrs(TempMNM, TempdNM, Msize);

		double *shapecoeff = new double[Msize-1];
		for(int i =0; i < Msize-1; i++){	
			shapecoeff[i] = TempdNM[i+1]/TempdNM[1];
		}

		for(int i =0; i < Msize-1; i++){	

			PDet += shapecoeff[i]*shapecoeff[i];
		}

		delete[] shapecoeff;

		if(debug ==1){printf("Size: %i %g \n", Msize, Margindet);}

		
		dgemv(M,TempdNM,shapevec,Nbins,Msize,'N');
		//dgemv(JitterBasis,shapecoeff,Jittervec,Nbins,Msize,'N');	



		double rms=0;
		

		for(int j =0; j < Nbins; j++){
			double datadiff =  (double)((MNStruct *)globalcontext)->TemplateChans[fc*Nbins + j] - shapevec[j];
			rms += datadiff*datadiff;
			if(debug ==1){printf("Data, Model: %i %i %g %g\n", fc, j, (double)((MNStruct *)globalcontext)->TemplateChans[fc*Nbins + j], shapevec[j]);}
		}

		rms = sqrt(rms/((double)Nbins));

		for(int j =0; j < Nbins; j++){
		    double datadiff =  (double)((MNStruct *)globalcontext)->TemplateChans[fc*Nbins + j] - shapevec[j];
		    Chisq += datadiff*datadiff/(rms*rms);
		    detN += log(rms*rms);
		}

		delete[] shapevec;
		//delete[] Jittervec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] TempdNM;

		if(debug ==1 || debug ==2){printf("End Like: %.10g %g %g %g\n", lnew, detN, Chisq, rms);}

	}


	profilelike = -0.5*(Chisq+detN);//-PDet;/
	lnew += profilelike;


	delete[] Betatimes;

	for (int j = 0; j < Nbins; j++){
	    delete[] Hermitepoly[j];
	    delete[] M[j];
	}
	delete[] Hermitepoly;
	delete[] M;

	for (int j = 0; j < Msize; j++){
	    delete[] MNM[j];
		delete[] TempMNM[j];
	}
	delete[] MNM;
	delete[] TempMNM;
	
	
	 
	 
	 
	 


	delete[] ProfileBats;
	delete[] numcoeff;
	delete[] CompSeps;

	if(debug ==1 || debug ==2){printf("End Like: %.10g %g %g\n", lnew, detN, Chisq);}

	if(debug ==1){sleep(5);}

	return lnew;

}


void WriteTemplate2DProfLike(std::string longname, int &ndim){

	//printf("In like \n");


        double *Cube = new double[ndim];
        int number_of_lines = 0;

        std::ifstream checkfile;
	std::string checkname;
	if(((MNStruct *)globalcontext)->sampler == 0){
	        checkname = longname+"phys_live.points";
	}
        if(((MNStruct *)globalcontext)->sampler == 1){
                checkname = longname+"_phys_live.txt";
        }
	printf("%i %s \n", ((MNStruct *)globalcontext)->sampler, checkname.c_str());
        checkfile.open(checkname.c_str());
        std::string line;
        while (getline(checkfile, line))
                ++number_of_lines;

        checkfile.close();

        std::ifstream summaryfile;
	std::string fname;
	if(((MNStruct *)globalcontext)->sampler == 0){
		fname = longname+"phys_live.points";
	}
	if(((MNStruct *)globalcontext)->sampler == 1){
	        fname = longname+"_phys_live.txt";
	}
        summaryfile.open(fname.c_str());



        printf("Getting ML \n");
        double maxlike = -1.0*pow(10.0,10);
        for(int i=0;i<number_of_lines;i++){

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);

                double like = paramlist[ndim];

                if(like > maxlike){
                        maxlike = like;
                         for(int i = 0; i < ndim; i++){
                                 Cube[i] = paramlist[i];
                         }
                }

        }
        summaryfile.close();
	printf("ML %g\n", maxlike);
	//printf("In like \n");


	int debug = 0;
	double TemplateFlux=0;
	double FakeRMS = 0;
	double TargetSN=0;
        long double LDparams;
        int pcount;

	double uniformpriorterm = 0;


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	LDparams=Cube[0]*(((MNStruct *)globalcontext)->LDpriors[0][1]) + (((MNStruct *)globalcontext)->LDpriors[0][0]);
	
	
	pcount=0;
	long double phase = LDparams*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

		   
	int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[0][1];

	long double *ProfileBats = new long double[Nbin];

	long double FoldingPeriod = ((MNStruct *)globalcontext)->ReferencePeriod;
	long double FoldingPeriodDays = FoldingPeriod/SECDAY;

	long double pulsesamplerate = FoldingPeriod/Nbin/SECDAY;

	for(int b =0; b < Nbin; b++){
		 ProfileBats[b] = pulsesamplerate*b;
	}

	TemplateFlux=0;


	int totalProfCoeff = 0;
	int *numcoeff= new int[((MNStruct *)globalcontext)->numProfComponents];

	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		numcoeff[i] =  ((MNStruct *)globalcontext)->numshapecoeff[i];
		totalProfCoeff += numcoeff[i];
	}

	double oldbeta=0.00373642;
	//double beta = oldbeta*((MNStruct *)globalcontext)->ReferencePeriod;
	double beta = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;


	//double Pscale = pow(10.0, 2*Cube[pcount]);
	double PDet = 0;
	pcount++;


	long double *CompSeps = new long double[((MNStruct *)globalcontext)->numProfComponents];
	CompSeps[0] = 0;
	for(int i = 1; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		CompSeps[i] = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;
		pcount++;
		//printf("CompSep: %g \n", Cube[pcount-1]);
	}

	if(debug ==1){printf("Params: %g %g \n", Cube[0], Cube[1]);}

	

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	double lnew = 0;
	int badshape = 0;


	


	int nTOA = 0;


	double logMargindet = 0;
	double Marginlike = 0;	 

	double profilelike=0;


	int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
	double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
	long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

	double *Betatimes = new double[Nbins];
	double **JitterBasis  =  new double*[Nbins];
	double **Hermitepoly =  new double*[Nbins];
	double **AllHermiteBasis = new double*[Nbins];

	int maxProfCoeff = totalProfCoeff+1;

        for(int i =0;i<Nbins;i++){AllHermiteBasis[i]=new double[maxProfCoeff];for(int j =0;j<maxProfCoeff;j++){AllHermiteBasis[i][j]=0;}}
	for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[totalProfCoeff];for(int j =0;j<totalProfCoeff;j++){Hermitepoly[i][j]=0;}}
	for(int i =0;i<Nbins;i++){JitterBasis[i]=new double[totalProfCoeff];for(int j =0;j<totalProfCoeff;j++){JitterBasis[i][j]=0;}}

	long double binpos = phase;




	long double minpos = binpos - FoldingPeriodDays/2;
	if(minpos < ProfileBats[0])minpos=ProfileBats[0];
	long double maxpos = binpos + FoldingPeriodDays/2;
	if(maxpos> ProfileBats[Nbins-1])maxpos =ProfileBats[Nbins-1];


	int cpos = 0;
	int jpos=0;
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		for(int j =0; j < Nbins; j++){
			long double timediff = 0;
			long double bintime = ProfileBats[j]+CompSeps[i]/SECDAY;
			if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
			}
			else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
			}
			else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
			}

			timediff=timediff*SECDAY;
		

			Betatimes[j]=(timediff)/beta;
			TNothplMC(numcoeff[i]+1,Betatimes[j],AllHermiteBasis[j], jpos);

			for(int k =0; k < numcoeff[i]+1; k++){

				double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));//*((MNStruct *)globalcontext)->Factorials[k]);

				AllHermiteBasis[j][k+jpos] = AllHermiteBasis[j][k+jpos]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);
				if(k<numcoeff[i]){ Hermitepoly[j][k+cpos] = AllHermiteBasis[j][k+jpos]; }

			}

			JitterBasis[j][0+cpos] = (1.0/sqrt(2.0))*(-1.0*AllHermiteBasis[j][1+jpos]);
			for(int k =1; k < numcoeff[i]; k++){
				JitterBasis[j][k+cpos] = (1.0/sqrt(2.0))*(sqrt(double(k))*AllHermiteBasis[j][k-1+jpos] - sqrt(double(k+1))*AllHermiteBasis[j][k+1+jpos]);
			}
				
		}
		cpos += numcoeff[i];
		jpos += numcoeff[i]+1;
   	 }

	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


	double **M = new double*[Nbins];

	int Msize = totalProfCoeff+1;
	for(int i =0; i < Nbins; i++){
		M[i] = new double[Msize];

		M[i][0] = 1;


		for(int j = 0; j < totalProfCoeff; j++){
			M[i][j+1] = Hermitepoly[i][j];
			//if(j==0)printf("%i %i %g \n", i, j, M[i][j+1]);
		}
	  
	}


	double **MNM = new double*[Msize];
	for(int i =0; i < Msize; i++){
	    MNM[i] = new double[Msize];
	}

	double **TempMNM = new double*[Msize];
	for(int i =0; i < Msize; i++){
	    TempMNM[i] = new double[Msize];
	}

	dgemm(M, M , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');

	for(int j = 0; j < Msize; j++){
		MNM[j][j] += pow(10.0,-14);
	}

	double Chisq=0;
	double detN = 0;

	for(int fc=0; fc < ((MNStruct *)globalcontext)->numTempFreqs; fc++){
	

		double *dNM = new double[Msize]();
		double *TempdNM = new double[Msize]();
		double *NDiffVec = new double[Nbins]();
		double *shapevec = new double[Nbins]();




		for(int j = 0; j < Nbins; j++){
			NDiffVec[j] = (double)((MNStruct *)globalcontext)->TemplateChans[fc*Nbins + j];
		}
		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
			for(int j =0; j < Msize; j++){
				TempMNM[i][j] = MNM[i][j];
			}
		}


		int info=0;
		double Margindet = 0;
		dpotrfInfo(TempMNM, Msize, Margindet, info);
		dpotri(TempMNM, Msize);
		dgemv(TempMNM,dNM,TempdNM,Msize,Msize,'N');

		double *shapecoeff = new double[Msize+1]();
		double *jcoeff = new double[Msize];
		double *jitterVec = new double[Nbins];
		double *ProfShape = new double[Nbins];
		double *jitterCheck = new double[Nbins];

		for(int i =0; i < Msize-1; i++){	
			shapecoeff[i] = TempdNM[i+1]/TempdNM[1];
		}

/*		jcoeff[0] =  (1.0/sqrt(2.0))*shapecoeff[0+1]*sqrt(0.0+1.0);
		for(int i =1; i < Msize; i++){
			jcoeff[i] =  (1.0/sqrt(2.0))*(shapecoeff[i+1]*sqrt(i+1.0) - shapecoeff[i-1]*sqrt(i));
		}
		for(int i =0; i < Msize; i++){
			jcoeff[i] = jcoeff[i]*pow(10.0, -7)/beta;	
		}
		//jcoeff[Msize-2] = shapecoeff[Msize - 2]  - (1.0/sqrt(2.0))*shapecoeff[Msize-2-1]*sqrt(Msize-2-1);

*/


		if(debug ==1){printf("Size: %i %g \n", Msize, Margindet);}

		
		dgemv(M,TempdNM,shapevec,Nbins,Msize,'N');



		//dgemv(JitterBasis,shapecoeff,jitterVec,Nbins,Msize-1,'N');
		dgemv(Hermitepoly,shapecoeff,ProfShape,Nbins,Msize-1,'N');
		//dgemv(AllHermiteBasis,jcoeff,jitterCheck,Nbins,Msize,'N');

/*
		double dot = 0;
		for(int j =0; j < Nbins; j++){
			jitterVec[j] = pow(10.0, -7)*jitterVec[j]/beta	;
			jitterCheck[j] = jitterCheck[j];			

			//if(fc==20){printf("PJ %i %i %g %g %g\n", fc, j, ProfShape[j], jitterVec[j], jitterCheck[j]);}
		}

		if(fc==20){
			double minflux = pow(10.0, 100);
			int minshift = 0;
			for(int j = -1000; j < 1001; j++){
				double sum=0;
				for(int i =0; i < Msize-1; i++){
					double cdiff = (shapecoeff[i] + jcoeff[i]*j/10.0);
					//if(j==1){printf("j=1, %i %g %g\n", i, shapecoeff[i] , jcoeff[i]*j/10.0);}
					sum += cdiff*cdiff;
				}
				//printf("%i %g \n", j,sum);
				if(sum < minflux){
					minflux = sum;
					minshift = j;
				}
				
			}
			//printf("min shift %i %g \n", minshift, minflux);
			for(int i =0; i < Msize; i++){
				//printf("%i %g %g \n", i, shapecoeff[i], shapecoeff[i] + jcoeff[i]*minshift/10.0);
			}
				
		}

*/
		double rms=0;
		

		for(int j =0; j < Nbins; j++){
			double datadiff =  (double)((MNStruct *)globalcontext)->TemplateChans[fc*Nbins + j] - shapevec[j];
			rms += datadiff*datadiff;
			if(debug ==1){printf("Data, Model: %i %i %g %g\n", fc, j, (double)((MNStruct *)globalcontext)->TemplateChans[fc*Nbins + j], shapevec[j]);}
		}

		rms = sqrt(rms/((double)Nbins));

		for(int j =0; j < Nbins; j++){
		    double datadiff =  (double)((MNStruct *)globalcontext)->TemplateChans[fc*Nbins + j] - shapevec[j];
		    Chisq += datadiff*datadiff/(rms*rms);
		    detN += log(rms*rms);
		}





		for(int j = 0; j < Msize-1; j++){
			printf("Coeff %i %i %g %g %g \n", fc, j, ((MNStruct *)globalcontext)->TemplateFreqs[fc], TempdNM[j+1]/TempdNM[1], sqrt(TempMNM[j+1][j+1]*rms*rms)/TempdNM[1]);
		}
		
		std::ofstream profilefile;
		char toanum[15];
		sprintf(toanum, "%d", fc);
		std::string ProfileName =  toanum;
		std::string dname = longname+ProfileName+"-Template.txt";
	
		profilefile.open(dname.c_str());
		for(int j =0; j < Nbins; j++){
			profilefile << j << " " << std::setprecision(10) << (double)((MNStruct *)globalcontext)->TemplateChans[fc*Nbins + j] << " " << shapevec[j] << " " << rms << " " << ProfShape[j] << "\n";
		}
    		profilefile.close();

		delete[] shapevec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] TempdNM;
		delete[] jitterVec;
		delete[] shapecoeff;	
		delete[] ProfShape;

		//printf("End Like: %i %.10g %g %g %g\n", fc, lnew, detN, Chisq, rms);
		if(debug ==1 || debug ==2){printf("End Like: %.10g %g %g %g\n", lnew, detN, Chisq, rms);}

	}

	printf("End Like0: %.10g %g %g\n", lnew, detN, Chisq);
	profilelike = -0.5*(Chisq+detN);
	lnew += profilelike;

/*
	delete[] Betatimes;

	for (int j = 0; j < Nbins; j++){
		delete[] AllHermiteBasis[j];
		delete[] Hermitepoly[j];
		delete[] M[j];
		delete[] JitterBasis[j];
	}
	delete[] Hermitepoly;
	delete[] M;
	delete[] AllHermiteBasis;
	delete[] JitterBasis;
*/
	printf("End Like0.5: %.10g %g %g\n", lnew, detN, Chisq);

	for (int j = 0; j < Msize; j++){
		delete[] MNM[j];
		delete[] TempMNM[j];
	}
	delete[] MNM;
	delete[] TempMNM;
	
	
	 
	 printf("End Like1: %.10g %g %g\n", lnew, detN, Chisq);
	 
	 


	delete[] ProfileBats;
	delete[] numcoeff;
	delete[] CompSeps;

	printf("End Like: %.10g %g %g\n", lnew, detN, Chisq);

	if(debug ==1){sleep(5);}


}
