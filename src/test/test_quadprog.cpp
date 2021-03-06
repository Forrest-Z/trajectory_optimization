#include <ros/ros.h>
#include <QuadProg++.hh>

/*
 File main.cc
 
 This file contains just an example on how to set-up the matrices for using with
 the solve_quadprog() function.
 
 The test problem is the following:
 
 Given:
 G =  4 -2   g0^T = [6 0]
     -2  4       
 
 Solve:
 min f(x) = 1/2 x G x + g0 x
 s.t.
   x_1 + x_2 = 3
   x_1 >= 0
   x_2 >= 0
   x_1 + x_2 >= 2
 
 The solution is x^T = [1 2] and f(x) = 12
 
 Author: Luca Di Gaspero
 DIEGM - University of Udine, Italy
 l.digaspero@uniud.it
 http://www.diegm.uniud.it/digaspero/
 
 Copyright 2006-2009 Luca Di Gaspero
 
 This software may be modified and distributed under the terms
 of the MIT license.  See the LICENSE file for details.
*/

int main(int argc, char **argv){
  quadprogpp::Matrix<double> G, CE, CI;
  quadprogpp::Vector<double> g0, ce0, ci0, x;
	int n, m, p;
	double sum = 0.0;
	char ch;
  
  n = 2;
  G.resize(n, n);
  {
		std::istringstream is("4, -2,"
													"-2, 4 ");

		for (int i = 0; i < n; i++)	
			for (int j = 0; j < n; j++)
				is >> G[i][j] >> ch;
				// G[i][j] = i*j;
	}
	
  g0.resize(n);
  {
		std::istringstream is("6.0, 0.0 ");

		for (int i = 0; i < n; i++)
			is >> g0[i] >> ch;
	}
  
  m = 1;
  CE.resize(n, m);
	{
		std::istringstream is("1.0, "
													"1.0 ");

		for (int i = 0; i < n; i++)
			for (int j = 0; j < m; j++)
				is >> CE[i][j] >> ch;
	} 
  
  ce0.resize(m);
	{
		std::istringstream is("-3.0 ");
		
		for (int j = 0; j < m; j++)
			is >> ce0[j] >> ch;
  }
	
	p = 3;
  CI.resize(n, p);
  {
		std::istringstream is("1.0, 0.0, 1.0, "
													"0.0, 1.0, 1.0 ");
  
		for (int i = 0; i < n; i++)
			for (int j = 0; j < p; j++)
				is >> CI[i][j] >> ch;
	}
  
  ci0.resize(p);
  {
		std::istringstream is("0.0, 0.0, -2.0 ");

		for (int j = 0; j < p; j++)
			is >> ci0[j] >> ch;
	}
  x.resize(n);

  std::cout << "f: " << solve_quadprog(G, g0, CE, ce0, CI, ci0, x) << std::endl;
	std::cout << "x: " << x << std::endl;
/*  for (int i = 0; i < n; i++)
    std::cout << x[i] << ' ';
	std::cout << std::endl;	 */

	/* FOR DOUBLE CHECKING COST since in the solve_quadprog routine the matrix G is modified */
	
	{
    std::istringstream is("4, -2,"
													"-2, 4 ");
	
		for (int i = 0; i < n; i++)
			for (int j = 0; j < n; j++)
				is >> G[i][j] >> ch;
	}
	
  std::cout << "Double checking cost: ";
	for (int i = 0; i < n; i++)
		for (int j = 0; j < n; j++)
			sum += x[i] * G[i][j] * x[j];
	sum *= 0.5;	
	
	for (int i = 0; i < n; i++)
		sum += g0[i] * x[i];
	std::cout << sum << std::endl;

	// test for matrix multiplication;
	quadprogpp::Matrix<double> M1, M2, M3, M4;
	quadprogpp::Vector<double> v1, v2;

	M1.resize(2, 2); 
	M2.resize(2, 2);

	v1.resize(2);
	v2.resize(2);

	M1[0][0] = 1;
	M1[0][1] = 2;
	M1[1][0] = 3;
	M1[1][1] = 4;

	M2[0][0] = 0;
	M2[0][1] = 1;
	M2[1][0] = 2;
	M2[1][1] = 3;

	M3.resize(2, 1);
	M3[0][0] = 3;
	M3[1][0] = 5;



	std::cout << M3 << std::endl;

	M4.resize(2, 1);
	M4[0][0] = 10;
	M4[1][0] = 5;

	quadprogpp::Matrix<double> M5 = quadprogpp::dot_prod(M1, M2);
	std::cout << M5 << std::endl;


	quadprogpp::Matrix<double> M6 = quadprogpp::t(M4);
	std::cout << M6 << std::endl;

	quadprogpp::Matrix<double> M7 = quadprogpp::dot_prod(M3, quadprogpp::t(M4));
	std::cout << M7 << std::endl;

	quadprogpp::Matrix<double> M8 = M7 * (double) 8;
	std::cout << M8 << std::endl;

	M8 = 0;
	std::cout << M8 << std::endl;

	quadprogpp::Vector<double> v3;
	quadprogpp::Matrix<double> M9;
	v3.resize(2); v3[0] = 1; v3[1] = 2;
	M9.resize(1, 2); M9.setRow(0, v3);
	std::cout << M9 << std::endl;

	quadprogpp::Matrix<double> M10;
	M10.resize(2, 2);
	M10[0][0] = 1; M10[0][1] = 2; M10[1][0] = 3; M10[1][1] = 4;
	std::cout << M10 << std::endl;
	M10.resize(4, 2);
	std::cout << M10 << std::endl;
}
