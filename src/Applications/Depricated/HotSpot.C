#include <Arg.h>
#include <EOS.h>
#include <Riemann_gen.h>

using namespace std;

int main(int, char **argv)
{
    ProgName(*argv);
    std::string file_;
    file_ = (getenv("EOSLIB_DATA_PATH") != NULL) ? getenv("EOSLIB_DATA_PATH") : "DATA ENV NOT SET!";
    file_ += "/test_data/ArrheniusTest.data";
    const char * file = file_.c_str();
    std::string libPath;
    libPath  = (getenv("EOSLIB_SHARED_LIBRARY_PATH") != NULL) ? getenv("EOSLIB_SHARED_LIBRARY_PATH") : "PATH ENV NOT SET!";
    const char * lib     = libPath.c_str();
    //const char *file     = "EOS.data";
    const char *material = "BirchMurnaghan::HMX";
    const char *units    = NULL;
    
    double T0  = 300;
    double P0  = 1e-4;
    double P1  = 1;
    double P2  = 20;
	int nsteps = 19;

    int ahead = 0;
    double fconverge = 1.0;
    
	while(*++argv)
	{
		GetVar(file,file);
		GetVar(files,file);
		GetVar(material,material);
		GetVar(units,units);
        GetVarValue(-ahead,ahead, 0);
        GetVarValue(ahead,ahead, 1);
        GetVar(fconverge,fconverge);
        
        
		GetVar(P0,P0);
		GetVar(T0,T0);
		GetVar(P1,P1);
		GetVar(P2,P2);
		GetVar(nsteps,nsteps);

	    ArgError;
	}
    
// set eos and initial state
	DataBase db;
	if( db.Read(file) )
		cerr << Error("Read failed" ) << Exit;
	
	EOS *eos = FetchEOS(material,db);
	if( !eos )
	    cerr << Error("material not found, ") << material << Exit;
    if( units && eos->ConvertUnits(units, db) )
		cerr << Error("ConvertUnits failed") << Exit;
    
	HydroState state0;        
	if( eos->PT(P0,T0,state0) )
	    cerr << Error("eos->PT(P0,T0) failed\n") << Exit;
    state0.u = 0.0;

    Hugoniot *hugoniot = eos->shock(state0);
    if( hugoniot == NULL )
        cerr << Error("eos->shock failed") << Exit;
    RiemannSolver_generic RP(*eos,*eos);
    
	cout.setf(ios::showpoint);
	cout.setf(ios::scientific, ios::floatfield);
	cout.setf(ios::left, ios::adjustfield);

    cout << Center(P_form, "Ps")   << " "
         << Center(u_form, "u")    << " "
         << Center(T_form, "Ts")   << " "
         << Center(u_form, "ujet") << " "
         << Center(u_form, "cjet") << " "
         << Center(T_form, "Tjet") << " ";
    WaveStateLabel(cout) << " "
         << Center(T_form, "T") << "\n";    
	const Units *u = eos->UseUnits();
    cout << Center(P_form, u->Unit("pressure"))    << " "
         << Center(u_form, u->Unit("velocity"))    << " "
         << Center(T_form, u->Unit("temperature")) << " "
         << Center(u_form, u->Unit("velocity"))    << " "
         << Center(u_form, u->Unit("velocity"))    << " "
         << Center(T_form, u->Unit("temperature")) << " ";
	WaveStateLabel(cout, *u) << " "
         << Center(T_form, u->Unit("temperature")) << "\n";
    

	double P = P1;
    double dP = (P2-P1)/nsteps;

    WaveState shock;
    WaveState jet;
    WaveState hotspot;
    WaveState left, right;
    for( int count = nsteps+1; count--; P += dP )
    {
       if( hugoniot->P(P,RIGHT,shock) )
	       cerr << Error("shock->jet failed\n") << Exit;
       cout << P_form << P             << " "
            << u_form << shock.u       << " "
            << T_form << eos->T(shock) << " ";
       
       Isentrope *isentrope = eos->isentrope(shock);
       if( isentrope == NULL )
           cerr << Error("eos->isentrope failed\n") << Exit;
       if( isentrope->u_escape(LEFT,jet) )
	       cerr << Error("isentrope failed\n") << Exit;
       cout << u_form << jet.u               << " "
            << u_form << eos->c(jet.V,jet.e) << " "
            << T_form << eos->T(jet)         << " ";

       RiemannSolver_generic solver(*eos,*eos);
       HydroState *state1 = &state0;
       if( ahead )
       {
            shock.u = 0;
            state1 = static_cast<HydroState *>(&shock);
       }
       jet.u *= fconverge;
       if( RP.Solve(jet, *state1, left,right) )
           cerr << Error("RP.Solve failed\n") << Exit;

       cout << left   << " "
            << T_form << eos->T(left) << "\n";
       delete isentrope;
    }
    
    return 0;
}
    
