/*
 * Copyright. Gaël Dottel, Christoph Hilken, and Jan Peleska 2016 - 2021
 *
 * Licensed under the EUPL V.1.1
 */

#include <iostream>
#include <fstream>
#include <memory>
#include <stdlib.h>
#include <string.h>

#include "interface/FsmPresentationLayer.h"
#include "fsm/Dfsm.h"
#include "fsm/Fsm.h"
#include "fsm/FsmNode.h"
#include "fsm/IOTrace.h"
#include "fsm/FsmPrintVisitor.h"
#include "fsm/FsmSimVisitor.h"
#include "fsm/FsmOraVisitor.h"
#include "trees/IOListContainer.h"
#include "trees/OutputTree.h"
#include "trees/TestSuite.h"
#include "json/json.h"


using namespace std;
using namespace Json;


/**
 *   Program execution parameters and associated types
 */
typedef enum {
    FSM_CSV,
    FSM_JSON,
    FSM_BASIC
} model_type_t;

typedef enum {
    WMETHOD,
    WPMETHOD,
    SAFE_WMETHOD,
    SAFE_WPMETHOD,
    SAFE_HMETHOD,
    HMETHOD,
    HSIMETHOD
} generation_method_t;


/** File containing the reference model */
static model_type_t modelType;
static string modelFile;

/** only for generation method SAFE_WPMETHOD */
static model_type_t modelAbstractionType;
static string modelAbstractionFile;
static string plStateFile;
static string plInputFile;
static string plOutputFile;
static string fsmName;
static string testSuiteFileName;
static string tcFilePrefix;
static generation_method_t genMethod;
static unsigned int numAddStates;

static shared_ptr<FsmPresentationLayer> pl = nullptr;
static shared_ptr<Dfsm> dfsm = nullptr;
static shared_ptr<Dfsm> dfsmAbstraction = nullptr;
static shared_ptr<Fsm> fsm = nullptr;
static shared_ptr<Fsm> fsmAbstraction = nullptr;

static bool isDeterministic = false;
static bool rttMbtStyle = false;


/**
 * Write program usage to standard error.
 * @param name program name as specified in argv[0]
 */
static void printUsage(char* name) {
    cerr << "usage: " << name << " [-w|-wp|-h|-hsi] [-s] [-n fsmname] [-p infile outfile statefile] [-a additionalstates] [-t testsuitename] [-rtt <prefix>] modelfile [model abstraction file]" << endl;
}

/**
 *  Determine the model type of a model specified in an *.fsm file.
 *
 *  @param modelFile Name of the file containing the model
 *
 *  @return FSM_BASIC, if the model file has extension .fsm and
 *          contains the low-level encoding.
 *
 *  @return FSM_JSON, if the model file has extension .fsm and
 *                    contains the JSON encoding.
 *
 *  @return FSM_CSV, if the model file has extension .csv
 *
 */
static model_type_t getModelType(const string& mf) {
    
    if ( mf.find(".csv") != string::npos ) {
        return FSM_CSV;
    }
    
    model_type_t t = FSM_BASIC;
    
    ifstream inputFile(mf);
    string line;
    getline(inputFile,line);
    // Basic encoding does not contain any { or [
    if ( line.find("{") != string::npos or
        line.find("[") != string::npos) {
        t = FSM_JSON;
    }
    
    inputFile.close();
    
    return t;
    
}

/**
 * Parse parameters, stop execution if parameters are illegal.
 *
 * @param argc parameter 1 from main() invocation
 * @param argv parameter 2 from main() invocation
 */
static void parseParameters(int argc, char* argv[]) {
    
    // Set default parameters
    genMethod = WPMETHOD;
    fsmName = string("FSM");
    testSuiteFileName = string("testsuite.txt");
    numAddStates = 0;
    
    bool haveModelFileName = false;
    
    for ( int p = 1; p < argc; p++ ) {
        
        if ( strcmp(argv[p],"-w") == 0 ) {
            switch (genMethod) {
                case WPMETHOD: genMethod = WMETHOD;
                    break;
                case SAFE_WPMETHOD: genMethod = SAFE_WMETHOD;
                    break;
                default:
                    break;
            }
        }
        else if ( strcmp(argv[p],"-wp") == 0 ) {
            if ( genMethod == SAFE_WMETHOD or
                genMethod == SAFE_WPMETHOD ) {
                genMethod= SAFE_WPMETHOD;
            }
            else {
                genMethod = WPMETHOD;
            }
        }
        else if ( strcmp(argv[p],"-h") == 0 ) {
            if ( genMethod == SAFE_WMETHOD or
                genMethod == SAFE_WPMETHOD or
                genMethod == SAFE_HMETHOD ) {
                genMethod= SAFE_HMETHOD;
            }
            else {
                genMethod = HMETHOD;
            }
        }
        else if ( strcmp(argv[p],"-hsi") == 0 ) {
            genMethod = HSIMETHOD;
        }
        else if ( strcmp(argv[p],"-s") == 0 ) {
            switch (genMethod) {
                case WPMETHOD: genMethod = SAFE_WPMETHOD;
                    break;
                case WMETHOD: genMethod = SAFE_WMETHOD;
                    break;
                case HMETHOD: genMethod = SAFE_HMETHOD;
                    break;
                default:
                    break;
            }
        }
        else if ( strcmp(argv[p],"-n") == 0 ) {
            if ( argc < p+2 ) {
                cerr << argv[0] << ": missing FSM name" << endl;
                printUsage(argv[0]);
                exit(1);
            }
            else {
                fsmName = string(argv[++p]);
            }
        }
        else if ( strcmp(argv[p],"-t") == 0 ) {
            if ( argc < p+2 ) {
                cerr << argv[0] << ": missing test suite name" << endl;
                printUsage(argv[0]);
                exit(1);
            }
            else {
                testSuiteFileName = string(argv[++p]);
            }
        }
        else if ( strcmp(argv[p],"-a") == 0 ) {
            if ( argc < p+2 ) {
                cerr << argv[0] << ": missing number of additional states" << endl;
                printUsage(argv[0]);
                exit(1);
            }
            else {
                numAddStates = atoi(argv[++p]);
            }
        }
        else if ( strcmp(argv[p],"-rtt") == 0 ) {
            if ( argc < p+2 ) {
                cerr << argv[0] << ": missing prefix for RTT-MBT test suite files" << endl;
                printUsage(argv[0]);
                exit(1);
            }
            else {
                rttMbtStyle = true;
                tcFilePrefix = string(argv[++p]);
            }
        }
        else if ( strcmp(argv[p],"-p") == 0 ) {
            if ( argc < p+4 ) {
                cerr << argv[0] << ": missing presentation layer files" << endl;
                printUsage(argv[0]);
                exit(1);
            }
            else {
                plInputFile = string(argv[++p]);
                plOutputFile = string(argv[++p]);
                plStateFile = string(argv[++p]);
            }
        }
        else if ( strstr(argv[p],".csv")  ) {
            haveModelFileName = true;
            modelFile = string(argv[p]);
            modelType = getModelType(modelFile);
        }
        else if ( strstr(argv[p],".fsm")  ) {
            haveModelFileName = true;
            modelFile = string(argv[p]);
            modelType = getModelType(modelFile);
        }
        else {
            cerr << argv[0] << ": illegal parameter `" << argv[p] << "'" << endl;
            printUsage(argv[0]);
            exit(1);
        }
        
        if ( haveModelFileName and
            (genMethod == SAFE_WPMETHOD or
             genMethod == SAFE_WMETHOD or
             genMethod == SAFE_HMETHOD) ) {
                p++;
                if ( p >= argc ) {
                    cerr << argv[0] << ": missing model abstraction file" << endl;
                    printUsage(argv[0]);
                    exit(1);
                }
                modelAbstractionFile = string(argv[p]);
                modelAbstractionType = getModelType(modelAbstractionFile); 
            }
        
    }
    
    if ( modelFile.empty() ) {
        cerr << argv[0] << ": missing model file" << endl;
        printUsage(argv[0]);
        exit(1);
    }
    
}


/**
 *   Instantiate DFSM or FSM from input file according to
 *   the different input formats which are supported.
 */
static void readModel(model_type_t mtp,
                      string thisFileName,
                      string thisFsmName,
                      shared_ptr<Fsm>& myFsm,
                      shared_ptr<Dfsm>& myDfsm) {
    
    myDfsm = nullptr;
    myFsm = nullptr;
    
    
    switch ( mtp ) {
        case FSM_CSV:
            isDeterministic = true;
            myDfsm = make_shared<Dfsm>(thisFileName,thisFsmName);
            pl = myDfsm->getPresentationLayer();
            
            break;
            
        case FSM_JSON:
        {
            Reader jReader;
            Value root;
            stringstream document;
            ifstream inputFile(modelFile);
            document << inputFile.rdbuf();
            inputFile.close();
            
            if ( jReader.parse(document.str(),root) ) {
                myDfsm = make_shared<Dfsm>(root);
                pl = myDfsm->getPresentationLayer();
            }
            else {
                cerr << "Could not parse JSON model - exit." << endl;
                exit(1);
            }
        }
            break;
            
        case FSM_BASIC:
            if ( plStateFile.empty() ) {
                pl = make_shared<FsmPresentationLayer>();
            }
            else {
                pl = make_shared<FsmPresentationLayer>(plInputFile,plOutputFile,plStateFile);
            }
            myFsm = make_shared<Fsm>(modelFile,pl,fsmName);
            if ( myFsm->isDeterministic() ) {
                isDeterministic = true;
                myDfsm = make_shared<Dfsm>(modelFile,pl,fsmName);
                myFsm = nullptr;
            }
            break;
    }
    
    if ( myFsm != nullptr ) {
        myFsm->toDot(fsmName);
    }
    else if ( myDfsm != nullptr ) {
        myDfsm->toDot(fsmName);
        myDfsm->toCsv(fsmName);
    }
    
}


static void readModelAbstraction(model_type_t mtp,
                                 string thisFileName,
                                 string thisFsmName,
                                 shared_ptr<Dfsm>& myDfsm,
                                 shared_ptr<FsmPresentationLayer> plRef) {
    
    myDfsm = nullptr;
    
    
    switch ( mtp ) {
        case FSM_CSV:
            isDeterministic = true;
            myDfsm = make_shared<Dfsm>(thisFileName,thisFsmName,plRef);
            break;
            
        case FSM_JSON:
        {
            Reader jReader;
            Value root;
            stringstream document;
            ifstream inputFile(thisFileName);
            document << inputFile.rdbuf();
            inputFile.close();
            
            if ( jReader.parse(document.str(),root) ) {
                myDfsm = make_shared<Dfsm>(root,plRef);
            }
            else {
                cerr << "Could not parse JSON model - exit." << endl;
                exit(1);
            }
        }
            break;
            
        case FSM_BASIC:
            cerr << "ERROR. Model abstraction for SAFE W/WP/H METHOD may only be specified in CSV or JSON format - exit." << endl;
            exit(1);
            break;
    }
    
    if ( myDfsm != nullptr ) {
        myDfsm->toDot("ABS_"+fsmName);
        myDfsm->toCsv("ABS_"+fsmName);
    }

}

typedef vector<int> TCTrace;
typedef pair < shared_ptr<InputTrace>, shared_ptr<InputTrace> > TracePair;

struct TCTraceHash
{
    size_t operator() (const vector<int> & itrc) const
    {
        string trace_string = "";
        for (int i : itrc)
        {
            trace_string += to_string(i);
        }

        return trace_string.size() ? stoi(trace_string) : 0 ;
    }
};

struct TracePairHash
{
    size_t operator() (const TracePair & tracePair) const
    {
        auto trc1 = tracePair.first;
        auto trc2 = tracePair.second;

        if (trc1->get().size() < trc2->get().size())
        {
            trc2 = tracePair.first;
            trc1 = tracePair.second;
        }

        string trace_string = "";
        for (int i : trc1->get())
        {
            trace_string += to_string(i);
        }
        for (int i : trc2->get())
        {
            trace_string += to_string(i);
        }

        return trace_string.size() ? stoi(trace_string) : 0 ;
    }
};

typedef pair < TracePair, TCTrace > TracePair2Trace;
typedef unordered_multimap < TracePair, TCTrace, TracePairHash > TC2TracesMap;

static void safeHMethod(shared_ptr<TestSuite> testSuite) {
    
    // Minimise original reference DFSM
    Dfsm dfsmRefMin = dfsm->minimise();

    // Minimise abstracted Dfsm
    Dfsm dfsmAbstractionMin = dfsmAbstraction->minimise();
    
    dfsmRefMin.toDot("FSM_MINIMAL");
    dfsmAbstractionMin.toDot("ABS_FSM_MINIMAL");
    dfsmAbstractionMin.toCsv("ABS_FSM_MINIMAL");
    cout << "REF    size = " << dfsm->size() << endl;
    cout << "REFMIN size = " << dfsmRefMin.size() << endl;
    cout << "ABSMIN size = " << dfsmAbstractionMin.size() << endl;
    
    shared_ptr<FsmNode> s0 = dfsmRefMin.getInitialState();
    shared_ptr<FsmPresentationLayer> pl = dfsmRefMin.getPresentationLayer();

    shared_ptr<Tree> iTreeH = dfsmRefMin.getStateCover();
    shared_ptr<Tree> iTreeSH = dfsmRefMin.getStateCover();

    // Let A be a set consisting of alpha.w, beta.w for any alpha != beta in V
    // and a distinguishing trace w. q0-after-alpha !~ q0-after-beta
    IOListContainer iolcV = iTreeH->getIOListsWithPrefixes();
    shared_ptr<vector<vector<int>>> iolV = iolcV.getIOLists();
    
    // Let B = V.(union_(i=1)^(m-n+1) Sigma_I)
    shared_ptr<Tree> B = dfsmRefMin.getStateCover();
    IOListContainer inputEnum = IOListContainer(dfsmRefMin.getMaxInput(),
                                                1,
                                                numAddStates + 1,
                                                pl);
    B->add(inputEnum);
    iTreeH->unionTree(B);
    iTreeSH->unionTree(B);

    TC2TracesMap tracePair2tc;

    vector<TracePair> tracesToCompare;

    // A
    shared_ptr<TreeNode> rootA = make_shared<TreeNode>();
    Tree treeA = Tree(rootA, pl);

    for (unsigned i = 0; i < iolV->size(); i++)
    {
        shared_ptr<InputTrace> alpha = make_shared<InputTrace>(iolV->at(i), pl);
        for (unsigned j = i + 1; j < iolV->size(); j++)
        {
            shared_ptr<InputTrace> beta = make_shared<InputTrace>(iolV->at(j), pl);
            tracesToCompare.push_back(make_pair(alpha,beta));

            shared_ptr<Tree> alphaTree = iTreeH->getSubTree(alpha);
            shared_ptr<Tree> betaTree = iTreeH->getSubTree(beta);

            shared_ptr<Tree> prefixRelationTree = alphaTree->getPrefixRelationTree(betaTree);

            InputTrace gamma = dfsmRefMin.calcDistinguishingTrace(alpha, beta, prefixRelationTree);

            shared_ptr<InputTrace> iAlphaGamma = make_shared<InputTrace>(alpha->get(), pl);
            iAlphaGamma->append(gamma.get());

            shared_ptr<InputTrace> iBetaGamma = make_shared<InputTrace>(beta->get(), pl);
            iBetaGamma->append(gamma.get());

            treeA.addToRoot(iAlphaGamma->get());
            treeA.addToRoot(iBetaGamma->get());

            iTreeH->addToRoot(iAlphaGamma->get());
            iTreeH->addToRoot(iBetaGamma->get());

            iTreeSH->addToRoot(iAlphaGamma->get());
            iTreeSH->addToRoot(iBetaGamma->get());
        }
    }


    // B
    shared_ptr<vector<vector<int>>> iolB = inputEnum.getIOLists();
    shared_ptr<FsmNode> abs_s0 = dfsmAbstractionMin.getInitialState();

    for (auto beta : *iolB)
    {
        for (auto alpha : *iolV)
        {
            shared_ptr<InputTrace> iAlphaBeta = make_shared<InputTrace>(alpha, pl);
            iAlphaBeta->append(beta);

            for (auto o : *iolV)
            {
                shared_ptr<InputTrace> iOmega = make_shared<InputTrace>(o, pl);

                shared_ptr<FsmNode> s0AfterAlphaBeta = *s0->after(*iAlphaBeta).begin();
                shared_ptr<FsmNode> s0AfterOmega = *s0->after(*iOmega).begin();
                if (s0AfterAlphaBeta == s0AfterOmega) continue;
                tracesToCompare.push_back(make_pair(iAlphaBeta,iOmega));
            }
        }
    }

    // C
    for (vector<int> b : *iolB)
    {
        shared_ptr<InputTrace> iAlphaBeta_2 = make_shared<InputTrace>(b, pl);

        for (unsigned i = 0; i < b.size()+1; i++)
        {
            vector<int> a(b.begin(), b.begin() + i);
            shared_ptr<InputTrace> iAlphaBeta_1 = make_shared<InputTrace>(a, pl);

            shared_ptr<FsmNode> s0AfterAlpha = *s0->after(*iAlphaBeta_1).begin();
            shared_ptr<FsmNode> s0AfterBeta = *s0->after(*iAlphaBeta_2).begin();
            if (s0AfterAlpha == s0AfterBeta) continue;
            tracesToCompare.push_back(make_pair(iAlphaBeta_1,iAlphaBeta_2));
        }
    }

    /* h method */
//    for (unsigned i = tracesToCompare.size(); i-->0; ) {
//        TracePair tracePair = tracesToCompare[i];
    for (TracePair tracePair : tracesToCompare)  {
        shared_ptr<InputTrace> alpha = tracePair.first;
        shared_ptr<InputTrace> beta = tracePair.second;

        shared_ptr<Tree> alphaTree = iTreeH->getSubTree(alpha);
        shared_ptr<Tree> betaTree = iTreeH->getSubTree(beta);
        shared_ptr<Tree> prefixRelationTree = alphaTree->getPrefixRelationTree(betaTree);
        InputTrace gamma = dfsmRefMin.calcDistinguishingTrace(alpha, beta, prefixRelationTree);

        shared_ptr<InputTrace> iAlphaGamma = make_shared<InputTrace>(alpha->get(), pl);
        iAlphaGamma->append(gamma.get());
        shared_ptr<InputTrace> iBetaGamma = make_shared<InputTrace>(beta->get(), pl);
        iBetaGamma->append(gamma.get());

        iTreeH->addToRoot(iAlphaGamma->get());
        iTreeH->addToRoot(iBetaGamma->get());

        TracePair2Trace tracePair2trace1(tracePair, iAlphaGamma->get());
        TracePair2Trace tracePair2trace2(tracePair, iBetaGamma->get());

        tracePair2tc.insert( tracePair2trace1 );
        tracePair2tc.insert( tracePair2trace2 );
    }

    /* safety h */
//    for (unsigned i = tracesToCompare.size(); i-->0; ) {
//        TracePair tracePair = tracesToCompare[i];
    for (TracePair tracePair : tracesToCompare)  {
        shared_ptr<InputTrace> alpha = tracePair.first;
        shared_ptr<InputTrace> beta = tracePair.second;

        shared_ptr<FsmNode> afterAlpha = *abs_s0->after(*alpha).begin();
        shared_ptr<FsmNode> afterBeta = *abs_s0->after(*beta).begin();
        if (afterAlpha == afterBeta) continue;

        // Each TracePair generates two test cases in h method.
        // Add those two test cases to safety-H-Tree
        auto range = tracePair2tc.equal_range(tracePair);
        auto it1 = range.first;
        auto it2 = it1;
        it2++;
        auto trc1 = it1->second;
        auto trc2 = it2->second;
        iTreeSH->addToRoot(trc1);
        iTreeSH->addToRoot(trc2);

    }

    IOListContainer iolc = iTreeSH->getIOLists();
    *testSuite = dfsmRefMin.createTestSuite(iolc);
}

static void safeWpMethod(shared_ptr<TestSuite> testSuite) {
    
    // Minimise original reference DFSM
    // Dfsm dfsmRefMin = dfsm->minimise();
    Fsm dfsmRefMin = dfsm->minimiseObservableFSM();
    
    dfsmRefMin.toDot("REFMIN");
    cout << "REF    size = " << dfsm->size() << endl;
    cout << "REFMIN size = " << dfsmRefMin.size() << endl;
    
    // Get state cover of original model
    shared_ptr<Tree> scov = dfsmRefMin.getStateCover();
    
    // Get transition cover of the original model
    shared_ptr<Tree> tcov = dfsmRefMin.getTransitionCover();
    
    // Get characterisation set of original model
    IOListContainer w = dfsmRefMin.getCharacterisationSet();
    
    cout << "W = " << w << endl;
    
    // Minimise the abstracted reference model
    Dfsm dfsmAbstractionMin = dfsmAbstraction->minimise();
    //Fsm dfsmAbstractionMin = dfsmAbstraction->minimiseObservableFSM();
    
    
    dfsmAbstractionMin.toDot("ABSMIN");
    cout << "ABSMIN size = " << dfsmAbstractionMin.size() << endl;
    
    
    // Get W_s, the characterisation set of dfsmAbstractionMin
    IOListContainer wSafe = dfsmAbstractionMin.getCharacterisationSet();
    
    cout << "wSafe = " << wSafe << endl;
    
    
    // Get W_sq, the state identification sets of dfsmAbstractionMin
    dfsmAbstractionMin.calcStateIdentificationSets();
    
    // Calc W1 = V.W, W from original model
    shared_ptr<Tree> W1 = dfsmRefMin.getStateCover();
    
    W1->add(w);
    
    // Calc W21 = V.wSafe
    shared_ptr<Tree> W2 = dfsmRefMin.getStateCover();
    W2->add(wSafe);
    
    // Calc W22 = V.(union_(i=1)^(m-n) Sigma_I).wSafe)
    shared_ptr<Tree> W22;
    if ( numAddStates > 0 ) {
        W22 = dfsmRefMin.getStateCover();
        IOListContainer inputEnum = IOListContainer(dfsm->getMaxInput(),
                                                    1,
                                                    numAddStates,
                                                    pl);
        W22->add(inputEnum);
        W22->add(wSafe);
        W2->unionTree(W22);
    }
    
    // Calc W3 = V.Sigma_I^(m - n + 1) oplus
    //           {Wis | Wis is state identification set of csmAbsMin}
    shared_ptr<Tree> W3 = dfsmRefMin.getStateCover();
    IOListContainer inputEnum2 = IOListContainer(dfsm->getMaxInput(),
                                                 (numAddStates+1),
                                                 (numAddStates+1),
                                                 pl);
    W3->add(inputEnum2);
    
    dfsmAbstractionMin.appendStateIdentificationSets(W3);
    
    // Union of all test cases: W1 union W2 union W3
    // Collected again in W1
    W1->unionTree(W2);
    W1->unionTree(W3);
    
    IOListContainer iolc = W1->getTestCases();
    *testSuite = dfsm->createTestSuite(iolc);
    
}

static void safeWMethod(shared_ptr<TestSuite> testSuite) {
    
    // Minimise original reference DFSM
    Dfsm dfsmRefMin = dfsm->minimise();
    
    cout << "REF    size = " << dfsm->size() << endl;
    cout << "REFMIN size = " << dfsmRefMin.size() << endl;
    
    // Get state cover of original model
    shared_ptr<Tree> scov = dfsmRefMin.getStateCover();
    
    // Get characterisation set of original model
    IOListContainer w = dfsmRefMin.getCharacterisationSet();
    
    cout << "W = " << w << endl;
    
    // Minimise the abstracted reference model
    Dfsm dfsmAbstractionMin = dfsmAbstraction->minimise();
    
    cout << "ABSMIN size = " << dfsmAbstractionMin.size() << endl;
    
    
    // Get W_s, the characterisation set of dfsmAbstractionMin
    IOListContainer wSafe = dfsmAbstractionMin.getCharacterisationSet();
    
    cout << "wSafe = " << wSafe << endl;
    
    // Calc W1 = V.W, W from original model
    shared_ptr<Tree> W1 = dfsmRefMin.getStateCover();
    
    W1->add(w);
    
    // Calc W21 = V.W_s
    shared_ptr<Tree> W21 = dfsmRefMin.getStateCover();
    W21->add(wSafe);
    
    // Calc W22 = V.(union_(i=1)^(m-n+1) Sigma_I).wSafe)
    shared_ptr<Tree> W22 = dfsmRefMin.getStateCover();
    
    IOListContainer inputEnum = IOListContainer(dfsm->getMaxInput(),
                                                1,
                                                numAddStates+1,
                                                pl);
    W22->add(inputEnum);
    
    W22->add(wSafe);
    
    // Union of all test cases: W1 union W2 union W3
    // Collected again in W1
    W1->unionTree(W21);
    W1->unionTree(W22);
    
    IOListContainer iolc = W1->getTestCases();
    *testSuite = dfsm->createTestSuite(iolc);
    
}



static void generateTestSuite() {
    
    shared_ptr<TestSuite> testSuite =
    make_shared<TestSuite>();
    
    switch ( genMethod ) {
        case WMETHOD:
            if ( dfsm != nullptr ) {
                IOListContainer iolc = dfsm->wMethod(numAddStates);
                for ( auto inVec : *iolc.getIOLists() ) {
                    shared_ptr<InputTrace> itrc = make_shared<InputTrace>(inVec,pl);
                    testSuite->push_back(dfsm->apply(*itrc));
                }
            }
            else {
                IOListContainer iolc = fsm->wMethod(numAddStates);
                for ( auto inVec : *iolc.getIOLists() ) {
                    shared_ptr<InputTrace> itrc = make_shared<InputTrace>(inVec,pl);
                    testSuite->push_back(fsm->apply(*itrc));
                }
            }
            break;
            
        case WPMETHOD:
            if ( dfsm != nullptr ) {
                IOListContainer iolc = dfsm->wpMethod(numAddStates);
                for ( auto inVec : *iolc.getIOLists() ) {
                    shared_ptr<InputTrace> itrc = make_shared<InputTrace>(inVec,pl);
                    testSuite->push_back(dfsm->apply(*itrc));
                }
            }
            else {
                IOListContainer iolc = fsm->wpMethod(numAddStates);
                for ( auto inVec : *iolc.getIOLists() ) {
                    shared_ptr<InputTrace> itrc = make_shared<InputTrace>(inVec,pl);
                    testSuite->push_back(fsm->apply(*itrc));
                }
            }
            break;
            
        case HMETHOD:
            if ( dfsm != nullptr ) {
                Dfsm dfsmMin = dfsm->minimise();
                IOListContainer iolc =
                dfsmMin.hMethodOnMinimisedDfsm(numAddStates);
                for ( auto inVec : *iolc.getIOLists() ) {
                    shared_ptr<InputTrace> itrc = make_shared<InputTrace>(inVec,pl);
                    testSuite->push_back(dfsm->apply(*itrc));
                }
            }
            break;
            
        case HSIMETHOD:
            if ( dfsm != nullptr ) {
                IOListContainer iolc = dfsm->hsiMethod(numAddStates);
                for ( auto inVec : *iolc.getIOLists() ) {
                    shared_ptr<InputTrace> itrc = make_shared<InputTrace>(inVec,pl);
                    testSuite->push_back(dfsm->apply(*itrc));
                }
            }
            else {
                IOListContainer iolc = fsm->hsiMethod(numAddStates);
                for ( auto inVec : *iolc.getIOLists() ) {
                    shared_ptr<InputTrace> itrc = make_shared<InputTrace>(inVec,pl);
                    testSuite->push_back(fsm->apply(*itrc));
                }
            }
            break;
            
        case SAFE_HMETHOD:
            safeHMethod(testSuite);
            break;
        case SAFE_WPMETHOD:
            safeWpMethod(testSuite);
            break;
            
        case SAFE_WMETHOD:
            safeWMethod(testSuite);
            break;
    }
    
    testSuite->save(testSuiteFileName);
    
    if ( rttMbtStyle ) {
        int numTc = 0;
        for ( size_t tIdx = 0; tIdx < testSuite->size(); tIdx++ ) {
            
            OutputTree ot = testSuite->at(tIdx);
            vector<IOTrace> iotrcVec;
            ot.toIOTrace(iotrcVec);
            
            for ( size_t iIdx = 0; iIdx < iotrcVec.size(); iIdx++ ) {
                ostringstream tcFileName;
                tcFileName << tcFilePrefix << tIdx << "_" << iIdx << ".log";
                ofstream outFile(tcFileName.str());
                outFile << iotrcVec[iIdx].toRttString();
                outFile.close();
                numTc++;
            }
            
        }
        
    }
    
    cout << "Number of test cases: " << testSuite->size() << endl;
    
}

int main(int argc, char* argv[])
{
    
    parseParameters(argc,argv);
    readModel(modelType,modelFile,fsmName,fsm,dfsm);
    
    if ( genMethod == SAFE_WPMETHOD or
        genMethod == SAFE_WMETHOD or
        genMethod == SAFE_HMETHOD) {
        if ( dfsm == nullptr ) {
            cerr << "SAFE W/WP METHOD only operates on deterministic FSMs - exit."
            << endl;
            exit(1);
        }
        
        shared_ptr<FsmPresentationLayer> plRef = dfsm->getPresentationLayer();
        
        readModelAbstraction(modelAbstractionType,
                             modelAbstractionFile,
                             "ABS_"+fsmName,
                             dfsmAbstraction,
                             plRef);
    }
    
    generateTestSuite();
    
    exit(0);
    
}



