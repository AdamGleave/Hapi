/*--------------------------------------------------------------------------*/
/*---------------------------- File Main.C ---------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * 
 * Sample Main file to illustrate the use of any solver deriving from
 * MCFClass. By changing just *two lines of code* and little more (see comment
 * PECULIARITY, if exists) the file works with any derived solver. 
 *
 * An instance of a Min Cost Flow problem in DIMACS standard format is read
 * from file and solved. In addition, the same problem can be written on a
 * file in MPS format. 
 *
 * \version 4.00
 *
 * \date 30 - 12 - 2009
 *
 * \author Alessandro Bertolini \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <fstream>
#include <sstream>
#include <string.h>

#include <boost/program_options.hpp>
#include <glog/logging.h>

#include "RelaxIV.h"
#define MCFSOLVER RelaxIV

// just change the two lines above and any MCFClass solver can be used (with
// the exception of PECULIARITY)

/*--------------------------------------------------------------------------*/
/*------------------------------- MACROS -----------------------------------*/
/*--------------------------------------------------------------------------*/

#define PRINT_RESULTS 1

/* If PRINT_RESULTS != 0, the optimal flows and potentials are printed
   after that the problem is successfully solved to optimality (so, watch
   out if your instance is very large). */

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

#if( OPT_USE_NAMESPACES )
 using namespace MCFClass_di_unipi_it;
#endif

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

template<class T>
inline T ABS( const T x )
{
 return( x >= T( 0 ) ? x : -x );
 }

/*--------------------------------------------------------------------------*/
// This function reads the first part of a string (before white spaces) and
// copy T value in the variable sthg (of T type)


template<class T>
static inline void str2val( const char* const str , T &sthg )
{
 istringstream( str ) >> sthg;
 }

/*--------------------------------------------------------------------------*/
// This function skips comment line in a input stream, where comment line is 
// marked by an initial '#' character

void SkipComments( ifstream &iParam , string &buf )
{
 do {
  iParam >> ws;
  getline( iParam , buf );
 }
 while( buf[ 0 ] == '#' );
}

/*--------------------------------------------------------------------------*/
// This function tries to read the parameter file; if it finds it, the
// corresponding parameters are set in the MCFClass object

void SetParam( MCFClass *mcf )
{
 ifstream iParam( "config.txt" ); 
 if( ! iParam.is_open() )
  return;

 string buf;
 int num;
 SkipComments( iParam , buf );
 str2val( buf.c_str(), num );        // get number of int parameters

 for( int i = 0 ; i < num ; i++ ) {  // read all int parameters
  int param , val;
  
  SkipComments( iParam , buf );
  str2val( buf.c_str(), param );     // parameter name
  
  SkipComments( iParam , buf );
  str2val( buf.c_str(), val );       // parameter value

  mcf->SetPar( param , val );

  }  // end( for( i ) )

 SkipComments( iParam , buf );
 str2val( buf.c_str() , num );       // get number of double parameters

 for( int i = 0 ; i < num ; i++ ) {  // read all double parameters
  int param;
  double val;
  SkipComments( iParam , buf );
  str2val( buf.c_str(), param );     // parameter name
  
  SkipComments( iParam , buf );
  str2val( buf.c_str() , val );      // parameter value
  
  mcf->SetPar( param , val );

  }  // end( for( i ) )
 }  // end( SetParam )

/*--------------------------------------------------------------------------*/
/*--------------------------------- MAIN -----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char **argv )
{
	// initialise logging
	FLAGS_logtostderr = true;
	google::InitGoogleLogging(argv[0]);

	// parse command line arguments
	namespace po = boost::program_options;

	po::options_description global("Global options");

	global.add_options()
		("help", "produce help message")
		("flow", po::value<bool>()->default_value(true), "Output flow solution.")
		("potentials", po::bool_switch()->default_value(false),
		 "Output dual solution. Must be used in conjunction with flow.");

	po::variables_map vm;

	po::parsed_options parsed = po::command_line_parser(argc, argv).
				options(global).
				run();
	po::store(parsed, vm);

	if (vm.count("help")) {
		std::cout << global << std::endl;
		return 0;
	}

	bool flow = vm["flow"].as<bool>();
	bool potentials = vm["potentials"].as<bool>();
 // reading command line parameters - - - - - - - - - - - - - - - - - - - - -

 if( argc == 2 && (strcmp(argv[1],"--help") == 0)) {
  cerr << "Usage: MCFSolve" << endl;
  return( -1 );
  }

 // opening input stream- - - - - - - - - - - - - - - - - - - - - - - - - - -
 istream &input = cin;


 try {
  // construct the solver - - - - - - - - - - - - - - - - - - - - - - - - - -

  MCFClass *mcf = new MCFSOLVER();

  mcf->SetMCFTime();  // do timing

  // load the network - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  mcf->LoadDMX(input);

  // set "reasonable" values for the epsilons, if any - - - - - - - - - - - -

  MCFClass::FNumber eF = 1;
  for( register MCFClass::Index i = mcf->MCFm() ; i-- ; )
   eF = max( eF , ABS( mcf->MCFUCap( i ) ) );

  for( register MCFClass::Index i = mcf->MCFn() ; i-- ; )
   eF = max( eF , ABS( mcf->MCFDfct( i ) ) );   

  MCFClass::CNumber eC = 1;
  for( register MCFClass::Index i = mcf->MCFm() ; i-- ; )
   eC = max( eC , ABS( mcf->MCFCost( i ) ) );

  mcf->SetPar( MCFSOLVER::kEpsFlw, (double) numeric_limits<MCFClass::FNumber>::epsilon() * eF *
		  mcf->MCFm() * 10);  // the epsilon for flows

  mcf->SetPar( MCFSOLVER::kEpsCst, (double) numeric_limits<MCFClass::CNumber>::epsilon() * eC *
		  mcf->MCFm() * 10);  // the epsilon for costs

  
  // set other parameters from configuration file (if any)- - - - - - - - - -

   SetParam( mcf );
  
  // solver call- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   mcf->SolveMCF();

  // output results - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  switch( mcf->MCFGetStatus() ) {
   case( MCFClass::kOK ):
    double tu , ts;
    mcf->TimeMCF( tu , ts );
    cerr << "Solution time (s): user " << tu << ", system " << ts << endl;

    #if( PRINT_RESULTS )
    {
    	if (flow) {
				if( ( numeric_limits<MCFClass::CNumber>::is_integer == 0 ) ||
					 ( numeric_limits<MCFClass::FNumber>::is_integer == 0 ) ) {
							cout.setf( ios::scientific, ios::floatfield );
							cout.precision( 12 );
							}

						 // cost of solution
						 cout << "s " << mcf->MCFGetFO() << endl;

						 MCFClass::Index m = mcf->MCFm();
						 MCFClass::FRow x = new MCFClass::FNumber[m];
						 MCFClass::Index_Set active_arcs = new MCFClass::Index[m];
						 mcf->MCFGetX(x, active_arcs);
						 MCFClass::Index_Set start = new MCFClass::Index[m];
						 MCFClass::Index_Set end = new MCFClass::Index[m];
						 mcf->MCFArcs(start, end, active_arcs);
						 for(MCFClass::Index i = 0;
								 active_arcs[i] != MCFClass::Inf<MCFClass::Index>(); i++) {
							 cout << "f " << start[i] << " " << end[i] << " " << x[i] << endl;
						 }
						 delete[] x;

						 if (potentials) {
							 MCFClass::CRow pi = new MCFClass::CNumber[ mcf->MCFn() ];
							 						 mcf->MCFGetPi( pi );
							 						 for( MCFClass::Index i = 0 ; i < mcf->MCFn() ; i++ )
							 							cout << "p " << i << " " << pi[ i ] << endl;
							 						 delete[] pi;
						 }
			  }
  }
  #endif
    // output overall time for benchmark suite
    cout << "c ALGORITHM TIME " << mcf->TimeMCF() << endl;
    // check solution
    mcf->CheckPSol();
    mcf->CheckDSol();

    break;
   case( MCFClass::kUnfeasible ):
    cout << "MCF problem unfeasible." << endl;
    break;
   case( MCFClass::kUnbounded ):
    cout << "MCF problem unbounded." << endl;
    break;
   default:
    cout << "Error in the MCF solver." << endl;
   }

  // destroy the object - - - - - - - - - - - - - - - - - - - - - - - - - - -

  delete( mcf );
  }
 // manage exceptions - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 catch( exception &e ) {
  cerr << e.what() << endl;
  return( 1 );
  }
 catch(...) {
  cerr << "Error: unknown exception thrown" << endl;
  return( 1 );
  }

 // terminate - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 return( 0 );

 }  // end( main )

/*--------------------------------------------------------------------------*/
/*------------------------- End File Main.C --------------------------------*/
/*--------------------------------------------------------------------------*/
