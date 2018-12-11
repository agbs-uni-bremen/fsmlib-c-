/*
 * Copyright. Gaël Dottel, Christoph Hilken, and Jan Peleska 2016 - 2021
 *
 * Licensed under the EUPL V.1.1
 */

#include <iostream>
#include <fstream>
#include <memory>
#include <stdlib.h>
#include <interface/FsmPresentationLayer.h>
#include <fsm/Dfsm.h>
#include <fsm/Fsm.h>
#include <fsm/FsmNode.h>
#include <fsm/IOTrace.h>
#include <fsm/FsmPrintVisitor.h>
#include <fsm/FsmSimVisitor.h>
#include <fsm/FsmOraVisitor.h>
#include <trees/IOListContainer.h>
#include <trees/OutputTree.h>
#include <trees/TestSuite.h>
#include "json/json.h"

#include "sets/HittingSet.h"
#include "sets/HsTreeNode.h"
#include <algorithm>
#include <cmath>
#include "fsm/PkTableRow.h"
#include "fsm/PkTable.h"
#include "fsm/DFSMTable.h"
#include "fsm/DFSMTableRow.h"
#include "fsm/OFSMTableRow.h"
#include "fsm/OFSMTable.h"
#include "fsm/FsmTransition.h"
#include "fsm/FsmLabel.h"


using namespace std;
using namespace Json;

void assertInconclusive(string tc, string comment = "") {
    
    string sVerdict("INCONCLUSIVE");
    cout << sVerdict << ": " << tc << " : " << comment <<  endl;
    
}

void fsmlib_assert(string tc, bool verdict, string comment = "") {
    
    string sVerdict = (verdict) ? "PASS" : "FAIL";
    cout << sVerdict << ": " << tc
    << " : "
    << comment <<  endl;
    
}

void test1() {
    
    cout << "TC-DFSM-0001 Show that Dfsm.applyDet() deals correctly with incomplete DFSMs "
    << endl;
    
    shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
    Dfsm d("../../../resources/TC-DFSM-0001.fsm",pl,"m1");
    d.toDot("TC-DFSM-0001");
    
    vector<int> inp;
    inp.push_back(1);
    inp.push_back(0);
    inp.push_back(0);
    inp.push_back(0);
    inp.push_back(1);
    
    
    InputTrace i(inp,pl);
    
    cout << "InputTrace = " << i << endl;
    
    
    IOTrace t = d.applyDet(i);
    
    cout << "IOTrace t = " << t << endl;
    
    vector<int> vIn = t.getInputTrace().get();
    vector<int> vOut = t.getOutputTrace().get();
    fsmlib_assert("TC-DFSM-0001",vIn.size() == 4
           and vOut.size() == 4
           and vOut[0] == 2
           and vOut[1] == 0
           and vOut[2] == 2
           and vOut[3] == 2,
           "For input trace 1.0.0.0.1, the output trace is 2.0.2.2");
    
    
    inp.insert(inp.begin(),9);
    InputTrace j(inp,pl);
    IOTrace u = d.applyDet(j);
    cout << "IOTrace u = " << u << endl;
    fsmlib_assert("TC-DFSM-0001",
           u.getOutputTrace().get().size() == 0 and
           u.getInputTrace().get().size() == 0,
           "For input trace 9, the output trace is empty.");
    
}


void test2() {
    
    cout << "TC-FSM-0001 Show that the copy constructor produces a deep copy of an FSM generated at random "
    << endl;
    
    shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
    shared_ptr<Fsm> f1 = Fsm::createRandomFsm("f1",3,5,10,pl);
    
    shared_ptr<Fsm> f2 = make_shared<Fsm>(*f1);
    
    f1->toDot("f1");
    f2->toDot("f1Copy");
    
    // Check using diff, that the dot-files of both FSMs
    // are identical
    fsmlib_assert("TC-FSM-0001", 0 == system("diff f1.dot f1Copy.dot"),
           "dot-files of original and copied FSM are identical");
    
    cout << "Show that original FSM and deep copy are equivalent, "
    << endl << "using the WpMethod";
    
    Fsm f1Obs = f1->transformToObservableFSM();
    Fsm f1Min = f1Obs.minimise();
    
    Fsm f2Obs = f2->transformToObservableFSM();
    Fsm f2Min = f2Obs.minimise();
    
    int m = (f2Min.getMaxNodes() > f1Min.getMaxNodes() ) ?
    (f2Min.getMaxNodes() - f1Min.getMaxNodes()) : 0;
    IOListContainer iolc = f1Min.wMethod(m);
    
    TestSuite t1 = f1Min.createTestSuite(iolc);
    TestSuite t2 = f2Min.createTestSuite(iolc);
    
    fsmlib_assert("TC-FSM-0001",
           t2.isEquivalentTo(t1),
           "Original FSM and its deep copy pass the same W-Method test suite");
    
    
    
}

void test3() {
    
    cout << "TC-FSM-0002 Show that createMutant() injects a fault into the original FSM" << endl;
    
    
    for ( size_t i = 0; i < 4; i++ ) {
        shared_ptr<FsmPresentationLayer> pl =
        make_shared<FsmPresentationLayer>();
        shared_ptr<Fsm> fsm = Fsm::createRandomFsm("F",5,5,8,pl,(unsigned)i);
        fsm->toDot("F");
        
        shared_ptr<Fsm> fsmMutant = fsm->createMutant("F_M",1,0);
        fsmMutant->toDot("FMutant");
        
        Fsm fsmMin = fsm->minimise();
        fsmMin.toDot("FM");
        
        Fsm fsmMutantMin = fsmMutant->minimise();
        
        unsigned int m = 0;
        if ( fsmMutantMin.getMaxNodes() > fsmMin.getMaxNodes() ) {
            m = fsmMutantMin.getMaxNodes() - fsmMin.getMaxNodes();
        }
        
        cout << "Call W-Method - additional states (m) = " << m << endl;
        
        IOListContainer iolc1 = fsmMin.wMethodOnMinimisedFsm(m);
        
        cout << "TS SIZE (W-Method): " << iolc1.size() << endl;
        
        if ( iolc1.size() > 1000) {
            cout << "Skip this test case, since size is too big" << endl;
            continue;
        }
        
        TestSuite t1 = fsmMin.createTestSuite(iolc1);
        TestSuite t2 = fsmMutantMin.createTestSuite(iolc1);
        
        fsmlib_assert("TC-FSM-0002", not t2.isEquivalentTo(t1),
               "Original FSM and mutant do not produce the same test suite results - tests are created by W-Method");
        
        IOListContainer iolc2 = fsmMin.wpMethod(m);
        
        cout << "TS SIZE (Wp-Method): " << iolc2.size() << endl;
        
        if ( iolc2.size() > iolc1.size() ) {
            
            ofstream outFile("fsmMin.fsm");
            fsmMin.dumpFsm(outFile);
            outFile.close();
             
            exit(1);
        }

        
        TestSuite t1wp = fsmMin.createTestSuite(iolc2);
        TestSuite t2wp = fsmMutantMin.createTestSuite(iolc2);
        
        fsmlib_assert("TC-FSM-0002",
               not t2wp.isEquivalentTo(t1wp),
               "Original FSM and mutant do not produce the same test suite results - tests are created by Wp-Method");
        
        fsmlib_assert("TC-FSM-0002",
               t1wp.size() <= t1.size(),
               "Wp-Method test suite size less or equal to W-Method size");
        
        if ( t1wp.size() > t1.size() ) {
            cout << "Test Suite Size (W-Method): " << t1.size()
            << endl << "Test Suite Size (Wp-Method): " << t1wp.size() << endl;
            cout << endl << "W-Method " << endl << iolc1 << endl;
            exit(1);
        }
        
        
    }
    
    
}


void test4() {
    
    cout << "TC-FSM-0004 Check correctness of state cover" << endl;
    
    const bool markAsVisited = true;
    bool havePassed = true;
    
    shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
    
    for (size_t i = 0; i < 2000; i++) {
        
        // Create a random FSM
        std::shared_ptr<Fsm> f = Fsm::createRandomFsm("F",5,5,10,pl,(unsigned)i);
        std::shared_ptr<Tree> sc = f->getStateCover();
        
        if ( sc->size() != (size_t)f->getMaxNodes() + 1 ) {
            cout << "Size of state cover: " << sc->size()
            << " Number of states in FSM: " << f->getMaxNodes() + 1 << endl;
            fsmlib_assert("TC-FSM-0004",
                   sc->size() <= (size_t)f->getMaxNodes() + 1,
                   "Size of state cover must be less or equal than number of FSM states");
        }
        
        
        IOListContainer c = sc->getTestCases();
        std::shared_ptr<std::vector<std::vector<int>>> iols = c.getIOLists();
        
        for ( auto inLst : *iols ) {
            auto iTr = make_shared<InputTrace>(inLst,pl);
            f->apply(*iTr,true);
        }
        
        for ( std::shared_ptr<FsmNode> n : f->getNodes() ) {
            if ( not n->hasBeenVisited() ) {
                havePassed = false;
                fsmlib_assert("TC-FSM-0004",
                       n->hasBeenVisited(),
                       "State cover failed to visit node " + n->getName());
                
                f->toDot("FailedStateCoverFSM");
                
                filebuf fb;
                fb.open ("FailedStateCover.dot",std::ios::out);
                ostream os(&fb);
                sc->toDot(os);
                fb.close();
                
                int iCtr = 0;
                for ( auto inLst : *iols ) {
                    ostringstream oss;
                    oss << iCtr++;
                    auto iTr = make_shared<InputTrace>(inLst,pl);
                    filebuf fbot;
                    OutputTree ot = f->apply(*iTr,markAsVisited);
                    fbot.open ("FailedStateCover" + oss.str() + ".dot",
                               std::ios::out);
                    ostream osdot(&fbot);
                    sc->toDot(osdot);
                    fbot.close();
                }
                
                exit(1);
                
            }
        }
        
    }
    
    if ( havePassed ) {
        fsmlib_assert("TC-FSM-0004",
               true,
               "State cover reaches all states");
    }
    else {
        exit(0);
    }
    
    
}

void test5() {
    
    cout << "TC-FSM-0005 Check correctness of input " <<
    "equivalence classes" << endl;
    
    shared_ptr<FsmPresentationLayer> pl =
    make_shared<FsmPresentationLayer>();
    
    
    shared_ptr<Fsm> fsm =
    make_shared<Fsm>("../../../resources/TC-FSM-0005.fsm",pl,"F");
    fsm->toDot("TC-FSM-0005");
    
    vector< std::unordered_set<int> > v = fsm->getEquivalentInputs();
    
    for ( size_t s = 0; s < v.size(); s++ ) {
        cout << s << ": { ";
        bool isFirst = true;
        for ( auto x : v[s] ) {
            if ( isFirst ) {
                isFirst= false;
            }
            else   {
                cout << ", ";
            }
            cout << x;
        }
        cout << " }" << endl;
    }
    
    fsmlib_assert("TC-FSM-0005",
           v.size() == 3,
           "For TC-FSM-0005.fsm, there are 3 classes of equivalent inputs.");
    
    fsmlib_assert("TC-FSM-0005",
           v[0].size() == 1 and v[0].find(0) != v[0].end(),
           "Class 0 only contains input 0.");
    
    fsmlib_assert("TC-FSM-0005",
           v[1].size() == 1 and v[1].find(1) != v[1].end(),
           "Class 1 only contains input 1.");
    
    fsmlib_assert("TC-FSM-0005",
           v[2].size() == 2 and
           v[2].find(2) != v[2].end() and
           v[2].find(3) != v[2].end(),
           "Class 2 contains inputs 2 and 3.");
    
    
    // Check FSM without any equivalent inputs
    fsm = make_shared<Fsm>("../../../resources/fsmGillA7.fsm",pl,"F");
    fsm->toDot("fsmGillA7");
    v = fsm->getEquivalentInputs();
    
    fsmlib_assert("TC-FSM-0005",
           v.size() == 3,
           "For fsmGillA7, there are 3 input classes.");
    
    bool ok = true;
    for ( size_t s=0; s < v.size() and ok; s++ ) {
        if ( v[s].size() != 1 or
            v[s].find((int)s) == v[s].end() ) {
            ok =false;
        }
    }
    
    fsmlib_assert("TC-FSM-0005",
           ok,
           "For fsmGillA7, class x just contains input x.");
    
}

void test6() {
    
    cout << "TC-FSM-0006 Check correctness of FSM Print Visitor " << endl;
    
    shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
    Dfsm d("../../../resources/TC-DFSM-0001.fsm",pl,"m1");
    
    FsmPrintVisitor v;
    
    d.accept(v);
    
    cout << endl << endl;
    assertInconclusive("TC-FSM-0006",
                       "Output of print visitor has to be checked manually");
    
    
}

void test7() {
    
    //    cout << "TC-FSM-0007 Check correctness of FSM Simulation Visitor "
    //    << endl;
    
    shared_ptr<FsmPresentationLayer> pl =
    make_shared<FsmPresentationLayer>("../../../resources/garageIn.txt",
                                      "../../../resources/garageOut.txt",
                                      "../../../resources/garageState.txt");
    Dfsm d("../../../resources/garage.fsm",pl,"GC");
    d.toDot("GC");
    
    FsmSimVisitor v;
    
    d.accept(v);
    
    v.setFinalRun(true);
    d.accept(v);
    
    cout << endl << endl;
    //    assertInconclusive("TC-FSM-0007",
    //                       "Output of simulation visitor has to be checked manually");
}


void test8() {
    
    //    cout << "TC-FSM-0008 Check correctness of FSM Oracle Visitor "
    //    << endl;
    
    shared_ptr<FsmPresentationLayer> pl =
    make_shared<FsmPresentationLayer>("../../../resources/garageIn.txt",
                                      "../../../resources/garageOut.txt",
                                      "../../../resources/garageState.txt");
    Dfsm d("../../../resources/garage.fsm",pl,"GC");
    d.toDot("GC");
    
    FsmOraVisitor v;
    
    d.accept(v);
    
    v.setFinalRun(true);
    d.accept(v);
    
    cout << endl << endl;
    //    assertInconclusive("TC-FSM-0008",
    //                       "Output of oracle visitor has to be checked manually");
}

void test9() {
    
    cout << "TC-FSM-0009 Check correctness of method removeUnreachableNodes() "
         << endl;
    
    shared_ptr<Dfsm> d = nullptr;
    Reader jReader;
    Value root;
    stringstream document;
    ifstream inputFile("../../../resources/unreachable_gdc.fsm");
    document << inputFile.rdbuf();
    inputFile.close();
    
    if ( jReader.parse(document.str(),root) ) {
        d = make_shared<Dfsm>(root);
    }
    else {
        cerr << "Could not parse JSON model - exit." << endl;
        exit(1);
    }
    
    
    d->toDot("GU");
    
    size_t oldSize = d->size();
    
    vector<shared_ptr<FsmNode>> uNodes;
    if ( d->removeUnreachableNodes(uNodes) ) {
        
        d->toDot("G_all_reachable");
        
        for ( auto n : uNodes ) {
            cout << "Removed unreachable node: " << n->getName() << endl;
        }
        
        fsmlib_assert("TC-FSM-0009",
               uNodes.size() == 2 and (oldSize - d->size()) == 2,
               "All unreachable states have been removed");
    }
    else {
        fsmlib_assert("TC-FSM-0009",
               false,
               "Expected removeUnreachableNodes() to return FALSE");
    }
    
    
}


void test10() {
    
    cout << "TC-FSM-0010 Check correctness of Dfsm::minimise() "
    << endl;
    
    shared_ptr<Dfsm> d = nullptr;
    shared_ptr<FsmPresentationLayer> pl;
    Reader jReader;
    Value root;
    stringstream document;
    ifstream inputFile("../../../resources/unreachable_gdc.fsm");
    document << inputFile.rdbuf();
    inputFile.close();
    
    if ( jReader.parse(document.str(),root) ) {
        d = make_shared<Dfsm>(root);
        pl = d->getPresentationLayer();
    }
    else {
        cerr << "Could not parse JSON model - exit." << endl;
        exit(1);
    }
    
    
    Dfsm dMin = d->minimise();
    
    IOListContainer w = dMin.getCharacterisationSet();
    
    shared_ptr<std::vector<std::vector<int>>> inLst = w.getIOLists();
    
    bool allNodesDistinguished = true;
    for ( size_t n = 0; n < dMin.size(); n++ ) {
        
        shared_ptr<FsmNode> node1 = dMin.getNodes().at(n);
        
        for ( size_t m = n+1; m < dMin.size(); m++ ) {
            shared_ptr<FsmNode> node2 = dMin.getNodes().at(m);
            
            bool areDistinguished = false;
            
            for ( auto inputs : *inLst ) {
                
                shared_ptr<InputTrace> itr = make_shared<InputTrace>(inputs,pl);
                
                OutputTree o1 = node1->apply(*itr);
                OutputTree o2 = node2->apply(*itr);
                
                if ( o1 != o2 ) {
                    areDistinguished = true;
                    break;
                }
                
            }
            
            if ( not areDistinguished ) {
                
                fsmlib_assert("TC-FSM-0010",
                       false,
                       "All nodes of minimised DFSM must be distinguishable");
                cout << "Could not distinguish nodes "
                << node1->getName() << " and " << node2->getName() << endl;
                
                allNodesDistinguished = false;
            }
            
        }
        
    }
    
    if ( allNodesDistinguished ) {
        fsmlib_assert("TC-FSM-0010",
               true,
               "All nodes of minimised DFSM must be distinguishable");
    }
    
}


void test10b() {
    
    cout << "TC-FSM-1010 Check correctness of Dfsm::minimise() with DFSM huang201711"
    << endl;
    
    shared_ptr<FsmPresentationLayer> pl =
        make_shared<FsmPresentationLayer>("../../../resources/huang201711in.txt",
                                          "../../../resources/huang201711out.txt",
                                          "../../../resources/huang201711state.txt");
    
    
    shared_ptr<Dfsm> d = make_shared<Dfsm>("../../../resources/huang201711.fsm",
                                           pl,
                                           "F");
    Dfsm dMin = d->minimise();
    
    IOListContainer w = dMin.getCharacterisationSet();
    
    shared_ptr<std::vector<std::vector<int>>> inLst = w.getIOLists();
    
    bool allNodesDistinguished = true;
    for ( size_t n = 0; n < dMin.size(); n++ ) {
        
        shared_ptr<FsmNode> node1 = dMin.getNodes().at(n);
        
        for ( size_t m = n+1; m < dMin.size(); m++ ) {
            shared_ptr<FsmNode> node2 = dMin.getNodes().at(m);
            
            bool areDistinguished = false;
            
            for ( auto inputs : *inLst ) {
                
                shared_ptr<InputTrace> itr = make_shared<InputTrace>(inputs,pl);
                
                OutputTree o1 = node1->apply(*itr);
                OutputTree o2 = node2->apply(*itr);
                
                if ( o1 != o2 ) {
                    areDistinguished = true;
                    break;
                }
                
            }
            
            if ( not areDistinguished ) {
                
                fsmlib_assert("TC-FSM-1010",
                       false,
                       "All nodes of minimised DFSM must be distinguishable");
                cout << "Could not distinguish nodes "
                << node1->getName() << " and " << node2->getName() << endl;
                
                allNodesDistinguished = false;
            }
            
        }
        
    }
    
    if ( allNodesDistinguished ) {
        fsmlib_assert("TC-FSM-1010",
               true,
               "All nodes of minimised DFSM must be distinguishable");
    }
    
}


void gdc_test1() {
    
    cout << "TC-GDC-0001 Check that the correct W-Method test suite "
    << endl << "is generated for the garage door controller example" << endl;

    
    shared_ptr<Dfsm> gdc =
    make_shared<Dfsm>("../../../resources/garage-door-controller.csv","GDC");
    
    shared_ptr<FsmPresentationLayer> pl = gdc->getPresentationLayer();
    
    gdc->toDot("GDC");
    gdc->toCsv("GDC");
    
    Dfsm gdcMin = gdc->minimise();
    
    gdcMin.toDot("GDC_MIN");
    
    IOListContainer iolc =
        gdc->wMethod(2);
    
    shared_ptr< TestSuite > testSuite =
        make_shared< TestSuite >();
    for ( auto inVec : *iolc.getIOLists() ) {
        shared_ptr<InputTrace> itrc = make_shared<InputTrace>(inVec,pl);
        testSuite->push_back(gdc->apply(*itrc));
    }
    
    int tcNum = 0;
    for ( auto iotrc : *testSuite ) {
        cout << "TC-" << ++tcNum << ": " << iotrc;
    }
    
    testSuite->save("testsuite.txt");
    
    fsmlib_assert("TC-GDC-0001",
            0 == system("diff testsuite.txt ../../../resources/gdc-testsuite.txt"),
           "Expected GDC test suite and generated suite are identical");
    
    
}




vector<IOTrace> runAgainstRefModel(shared_ptr<Dfsm> refModel,
                                   IOListContainer& c) {
    
    shared_ptr<FsmPresentationLayer> pl = refModel->getPresentationLayer();
    
    auto iolCnt = c.getIOLists();
    
    // Register test cases in IO Traces
    vector<IOTrace> iotrLst;

    for ( auto lst : *iolCnt ) {
        
        shared_ptr<InputTrace> itr = make_shared<InputTrace>(lst,pl);
        IOTrace iotr = refModel->applyDet(*itr);
        iotrLst.push_back(iotr);
        
    }
    
    return iotrLst;
    
}

void runAgainstMutant(shared_ptr<Dfsm> mutant, vector<IOTrace>& expected) {
    
    for ( auto io : expected ) {
        
        InputTrace i = io.getInputTrace();
        
        if ( not mutant->pass(io) ) {
            cout << "FAIL: expected " << io << endl
            << "     : observed " << mutant->applyDet(i) << endl;
        }
        else {
            cout << "PASS: " << i << endl;
        }
        
    }
    
}

void wVersusT() {
    
    shared_ptr<Dfsm> refModel = make_shared<Dfsm>("FSBRTSX.csv","FSBRTS");

//    IOListContainer wTestSuite0 = refModel->wMethod(0);
//    IOListContainer wTestSuite1 = refModel->wMethod(1);
//    IOListContainer wTestSuite2 = refModel->wMethod(2);
//    IOListContainer wTestSuite3 = refModel->wMethod(3);
//    
  IOListContainer wpTestSuite0 = refModel->wpMethod(0);
//    IOListContainer wpTestSuite1 = refModel->wpMethod(1);
//    IOListContainer wpTestSuite2 = refModel->wpMethod(2);
//    IOListContainer wpTestSuite3 = refModel->wpMethod(3);
    
    //    IOListContainer tTestSuite = refModel->tMethod();
    
//    vector<IOTrace> expectedResultsW0 = runAgainstRefModel(refModel, wTestSuite0);
//    vector<IOTrace> expectedResultsW1 = runAgainstRefModel(refModel, wTestSuite1);
//    vector<IOTrace> expectedResultsW2 = runAgainstRefModel(refModel, wTestSuite2);
//    vector<IOTrace> expectedResultsW3 = runAgainstRefModel(refModel, wTestSuite3);
    vector<IOTrace> expectedResultsWp0 = runAgainstRefModel(refModel, wpTestSuite0);
//    vector<IOTrace> expectedResultsWp1 = runAgainstRefModel(refModel, wpTestSuite1);
//    vector<IOTrace> expectedResultsWp2  = runAgainstRefModel(refModel, wpTestSuite2);
//    vector<IOTrace> expectedResultsWp3 = runAgainstRefModel(refModel, wpTestSuite3);
//    vector<IOTrace> expectedResultsT = runAgainstRefModel(refModel, tTestSuite);


    
    for ( int i = 0; i < 10; i++ ) {
        
        cout << "Mutant No. " << (i+1) << ": " << endl;
        
        shared_ptr<Dfsm> mutant =
            make_shared<Dfsm>("FSBRTSX.csv","FSBRTS");
        mutant->createAtRandom();
        
//        runAgainstMutant(mutant,expectedResultsW0);
//        runAgainstMutant(mutant,expectedResultsW1);
//        runAgainstMutant(mutant,expectedResultsW2);
//        runAgainstMutant(mutant,expectedResultsW3);
        runAgainstMutant(mutant,expectedResultsWp0);
//        runAgainstMutant(mutant,expectedResultsWp1);
//        runAgainstMutant(mutant,expectedResultsWp2);
//        runAgainstMutant(mutant,expectedResultsWp3);
//        runAgainstMutant(mutant,expectedResultsT);

        
    }
    
    
}

void test11() {
    
    shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>("../../../resources/garageIn.txt",
                                                                            "../../../resources/garageOut.txt",
                                                                            "../../../resources/garageState.txt");
    
    shared_ptr<Fsm> gdc = make_shared<Fsm>("../../../resources/garage.fsm",pl,"GDC");
    
    
    gdc->toDot("GDC");
    
    Fsm gdcMin = gdc->minimise();
    
    gdcMin.toDot("GDC_MIN");
    
}

void test12() {
    
    shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>("../../../resources/garageIn.txt",
                                                                            "../../../resources/garageOut.txt",
                                                                            "../../../resources/garageState.txt");
    
    shared_ptr<Dfsm> gdc = make_shared<Dfsm>("../../../resources/garage.fsm",pl,"GDC");
    
    
    gdc->toDot("GDC");
    
    Dfsm gdcMin = gdc->minimise();
    
    gdcMin.toDot("GDC_MIN");
    
}

void test13() {
    
    shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
    
    shared_ptr<Dfsm> gdc = make_shared<Dfsm>("../../../resources/garage.fsm",pl,"GDC");
    
    
    gdc->toDot("GDC");
    
    Dfsm gdcMin = gdc->minimise();
    
    gdcMin.toDot("GDC_MIN");
    
}


void test14() {
    
    shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
    
    shared_ptr<Fsm> fsm = make_shared<Fsm>("../../../resources/NN.fsm",pl,"NN");
    
    fsm->toDot("NN");
    
    Fsm fsmMin = fsm->minimiseObservableFSM();
    
    fsmMin.toDot("NN_MIN");
    
}


void test15() {
    
    cout << "TC-DFSM-0015 Show that Fsm::transformToObservableFSM() produces an "
    << "equivalent observable FSM"
    << endl;
    
    shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
    
    shared_ptr<Fsm> nonObs = make_shared<Fsm>("../../../resources/nonObservable.fsm",pl,"NON_OBS");
    
    
    nonObs->toDot("NON_OBS");
    
    Fsm obs = nonObs->transformToObservableFSM();
    
    obs.toDot("OBS");
    
    fsmlib_assert("TC-DFSM-0015",
           obs.isObservable(),
           "Transformed FSM is observable");
    
    // Show that nonObs and obs have the same language.
    // We use brute force test that checks all traces of length n*m
    int n = (int)nonObs->size();
    int m = (int)obs.size();
    int theLen = n+m-1;
    
    IOListContainer allTrc = IOListContainer(nonObs->getMaxInput(),
                                             1,
                                             theLen,
                                             pl);
    
    shared_ptr<vector<vector<int>>> allTrcLst = allTrc.getIOLists();
    
    for ( auto trc : *allTrcLst ) {
        
        // Run the test case against both FSMs and compare
        // the (nondeterministic) result
        shared_ptr<InputTrace> iTr =
        make_shared<InputTrace>(trc,pl);
        OutputTree o1 = nonObs->apply(*iTr);
        OutputTree o2 = obs.apply(*iTr);
        
        if ( o1 != o2 ) {
            
            fsmlib_assert("TC-DFSM-0015",
                   o1 == o2,
                   "Transformed FSM has same language as original FSM");
            
            cout << "o1 = " << o1 << endl;
            cout << "o2 = " << o2 << endl;
            return;
            
        }
       
    }
    
}

void faux() {
    
    
    shared_ptr<FsmPresentationLayer> pl =
    make_shared<FsmPresentationLayer>("../../../resources/gillIn.txt",
                                      "../../../resources/gillOut.txt",
                                      "../../../resources/gillState.txt");
    
    shared_ptr<Dfsm> d = make_shared<Dfsm>("../../../resources/gill.fsm",
                                           pl,
                                           "G0");
    
    d->toDot("G0");
    
    d->toCsv("G0");
    
    Dfsm dMin = d->minimise();
    
    dMin.toDot("G0_MIN");
    
    
    
}

void test16() {
    
    shared_ptr<Dfsm> exp1 = nullptr;
    Reader jReader;
    Value root;
    stringstream document;
    ifstream inputFile("../../../resources/exp1.fsm");
    document << inputFile.rdbuf();
    inputFile.close();
    
    if ( jReader.parse(document.str(),root) ) {
        exp1 = make_shared<Dfsm>(root);
    }
    else {
        cerr << "Could not parse JSON model - exit." << endl;
        exit(1);
    }
    
    exp1->toDot("exp1");
    
    shared_ptr<Dfsm> exp2 = nullptr;
    Reader jReader2;
    Value root2;
    stringstream document2;
    ifstream inputFile2("../../../resources/exp2.fsm");
    document2 << inputFile2.rdbuf();
    inputFile2.close();
    
    if ( jReader2.parse(document2.str(),root) ) {
        exp2 = make_shared<Dfsm>(root);
    }
    else {
        cerr << "Could not parse JSON model - exit." << endl;
        exit(1);
    }
    
    exp2->toDot("exp2");
    
    Fsm prod = exp1->intersect(*exp2);
    
    cout << endl << "NEW PL STATES" << endl ;
    prod.getPresentationLayer()->dumpState(cout);
    
    
    prod.toDot("PRODexp1exp2");
    
    
    
    
}

/*
	Calculates the set of transitions labels of outgoing transitions from given nodes set.
*/
unordered_set<FsmLabel> calcLblSet(std::unordered_set < shared_ptr<FsmNode>> &nodes) {
	unordered_set<FsmLabel> lblSet;
	for (auto n : nodes) {
		for (auto tr : n->getTransitions()) {
			lblSet.insert(*tr->getLabel());
		}
	}
	return lblSet;
}

/*
	Checks if processed contains front.
*/
//bool containsPair(std::vector<std::pair<std::unordered_set<std::shared_ptr<FsmNode>>, std::unordered_set<std::shared_ptr<FsmNode>>>> &processed,
//	std::pair<std::unordered_set<std::shared_ptr<FsmNode>>, std::unordered_set<std::shared_ptr<FsmNode>>> &front) {
//	for (auto p : processed) {
//		if (p.first == front.first && p.second == front.second) {
//			return true;
//		}
//	}
//	return false;
//}

/*
	Checks if wl contains pair.
*/
//bool containsPair(std::deque<std::pair<std::unordered_set<std::shared_ptr<FsmNode>>, std::unordered_set<std::shared_ptr<FsmNode>>>> &wl,
//	std::pair<std::unordered_set<std::shared_ptr<FsmNode>>, std::unordered_set<std::shared_ptr<FsmNode>>> &pair) {
//	for (auto p : wl) {
//		if (p.first == pair.first && p.second == pair.second) {
//			return true;
//		}
//	}
//	return false;
//}

template<typename T>
bool containsPair(T &lst, std::pair<std::unordered_set<std::shared_ptr<FsmNode>>, std::unordered_set<std::shared_ptr<FsmNode>>> &pair)
{
	typename T::const_iterator it;
	for (it = lst.begin(); it != lst.end(); ++it)
	{                                      
		if (it->first == pair.first && it->second == pair.second) {
			return true;
		}
	}
	return false;
}

/*
	Calculates and returns the set of target nodes reached from states contained in given node list with transitions labeled with given lbl.
*/
unordered_set<shared_ptr<FsmNode>> calcTargetNodes(unordered_set<shared_ptr<FsmNode>> &nodes, FsmLabel &lbl) {
	unordered_set<shared_ptr<FsmNode>> tgtNds;
	for (auto n : nodes) {
		for (auto tr : n->getTransitions()) {
			if (*tr->getLabel() == lbl) tgtNds.insert(tr->getTarget());
		}
	}
	return tgtNds;
}

/**
	This function returns true iff L(q) = L(u). Otherwise the function returns false.
*/
bool ioEquivalenceCheck(std::shared_ptr<FsmNode> q, std::shared_ptr<FsmNode> u) {
	// init wl with (q,u)
	std::deque<std::pair<std::unordered_set<std::shared_ptr<FsmNode>>, std::unordered_set<std::shared_ptr<FsmNode>>>> wl;
	std::unordered_set<std::shared_ptr<FsmNode>> l = std::unordered_set<std::shared_ptr<FsmNode>>{q};
	std::unordered_set<std::shared_ptr<FsmNode>> r = std::unordered_set<std::shared_ptr<FsmNode>>{u};	
	//wl.push_back(std::pair<std::unordered_set<std::shared_ptr<FsmNode>>, std::unordered_set<std::shared_ptr<FsmNode>>>(first, second));
	wl.push_back({ l,r });
	std::vector<std::pair<std::unordered_set<std::shared_ptr<FsmNode>>, std::unordered_set<std::shared_ptr<FsmNode>>>> processed;

	while (not wl.empty()) {
		auto front = wl.front();
		wl.pop_front();
		//calculate lblSet_l
		unordered_set<FsmLabel> lblSet_l = calcLblSet(front.first);
		//calculate lblSet_r
		unordered_set<FsmLabel> lblSet_r = calcLblSet(front.second);
		if (lblSet_l != lblSet_r) return false;

		// insert front to processed if processed does not contain front already
		if (not containsPair(processed, front)) {
			processed.push_back(front);
		}

		for (auto lbl : lblSet_l) {
			// calc tgtNds_l
			unordered_set<shared_ptr<FsmNode>> tgtNds_l = calcTargetNodes(front.first, lbl);
			// calc tgtNds_r
			unordered_set<shared_ptr<FsmNode>> tgtNds_r = calcTargetNodes(front.second, lbl);
			pair<unordered_set<shared_ptr<FsmNode>>, unordered_set<shared_ptr<FsmNode>>> pair{tgtNds_l, tgtNds_r};
			if (not containsPair(wl, pair) and not containsPair(processed, pair)) {
				wl.push_back(pair);
			}
		}
	}
	return true;
}

/*
	Checks if given fsm contains a pair of states with the same language. Returns true iff fsm contains states q, q' with
	q != q' and L(q) = L(q').
*/
bool hasEquivalentStates(Fsm &fsm) {
	for (int c1 = 0; c1 < fsm.getNodes().size(); c1++) {
		for (int c2 = c1 + 1; c2 < fsm.getNodes().size(); c2++) {
			if (ioEquivalenceCheck(fsm.getNodes().at(c1), fsm.getNodes().at(c2))) {
				return true;
			}
		}
	}
	return false;
}

/*
	Check if fsm1 and fsm2 have the same structure (same labeled transitions between nodes with the same indices).
	This method can be used to check if some Fsms structure was changed by some method call.
	Its faster than checking for isomorphism, because of restrictiveness.
	In other words this function tests if fsm1 and fsm2 have identical nodes lists.
*/
bool checkForEqualStructure(const Fsm &fsm1, const Fsm &fsm2) {
	// fsm1 and fsm2 need to be the same size
	if (fsm1.getNodes().size() != fsm2.getNodes().size()) return false;
	for (int i = 0; i < fsm1.getNodes().size(); ++i) {
		// each node should have the same number of transitions
		if (fsm1.getNodes().at(i)->getTransitions().size() != fsm2.getNodes().at(i)->getTransitions().size()) return false;
		for (int j = 0; j < fsm1.getNodes().at(i)->getTransitions().size(); ++j) {
			auto fsm1Tr = fsm1.getNodes().at(i)->getTransitions().at(j);
			auto fsm2Tr = fsm2.getNodes().at(i)->getTransitions().at(j);
			// compare fsm1Tr and fsm2Tr
			if (fsm1Tr->getSource()->getId() != fsm2Tr->getSource()->getId()
				|| fsm1Tr->getTarget()->getId() != fsm2Tr->getTarget()->getId()
				|| not (*fsm1Tr->getLabel() == *fsm2Tr->getLabel())) {
				return false;
			}
		}
	}
	return true;
}

/*
	Returns a set of all the states of the given Fsm that are reachable from the initial state of that Fsm.
*/
unordered_set<shared_ptr<FsmNode>> getReachableStates(const Fsm &fsm) {
	unordered_set<shared_ptr<FsmNode>> reached{fsm.getInitialState()};
	deque<shared_ptr<FsmNode>> wl{ fsm.getInitialState() };
	while (not wl.empty()) {
		shared_ptr<FsmNode> q = wl.front();
		wl.pop_front();
		for (auto tr : q->getTransitions()) {
			// add reached target to wl if this target wasn't reached before
			if (reached.insert(tr->getTarget()).second) {
				wl.push_back(tr->getTarget());
			}
		}
	}
	return reached;
}

/*
	Returns true iff nodes[i].id == i for all 0 <= i < fsm.getNodes().size()
*/
bool checkNodeIds(Fsm &fsm) {
	for (size_t i = 0; i < fsm.getNodes().size(); ++i) {
		if (fsm.getNodes().at(i)->getId() != i) return false;
	}
	return true;
}

/*
	Returns true iff fsm.getNodes() contains given node pointer.
*/
bool contains(Fsm &fsm, shared_ptr<FsmNode> node) {
	for (auto n : fsm.getNodes()) {
		if (n == node) return true;
	}
	return false;
}


/*
	Checks the transitions and return false iff any transitions hurts the invariant of Fsm.
*/
bool checkAllTransitions(Fsm &fsm) {
	for (auto n : fsm.getNodes()) {
		for (auto tr : n->getTransitions()) {
			if (tr == nullptr || tr->getLabel() == nullptr || tr->getLabel()->getInput() > fsm.getMaxInput()
				|| tr->getLabel()->getOutput() > fsm.getMaxOutput() || tr->getSource() != n
				|| not contains(fsm, tr->getTarget())) {
				return false;
			}
		}
	}
	return true;
}

/*
	This function checks the Fsm class invariant for the given Fsm object.
*/
bool checkFsmClassInvariant(Fsm &fsm) {
	if (fsm.getMaxInput() < 0) return false;
	if (fsm.getMaxOutput() < 0) return false;
	if (fsm.getNodes().size() < 1) return false;
	if (not checkNodeIds(fsm)) return false;
	if (contains(fsm, nullptr)) return false;
	if (not checkAllTransitions(fsm)) return false;
	if (fsm.getMaxState() != fsm.getNodes().size() - 1) return false;
	if (not(0 <= fsm.getInitStateIdx() and fsm.getInitStateIdx() <= fsm.getMaxState())) return false;
	return true;
}


void testIOEquivalenceCheck() {
	{
	shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
	shared_ptr<FsmNode> q0 = make_shared<FsmNode>(0, pl);
	shared_ptr<FsmNode> q1 = make_shared<FsmNode>(1, pl);

	shared_ptr<FsmTransition> tr0 = make_shared<FsmTransition>(q0, q1, make_shared<FsmLabel>(0, 1, pl));
	q0->addTransition(tr0);
	shared_ptr<FsmTransition> tr1 = make_shared<FsmTransition>(q1, q1, make_shared<FsmLabel>(1, 1, pl));
	q1->addTransition(tr1);

	shared_ptr<FsmNode> u0 = make_shared<FsmNode>(0, pl);
	shared_ptr<FsmNode> u1 = make_shared<FsmNode>(1, pl);
	shared_ptr<FsmNode> u2 = make_shared<FsmNode>(2, pl);

	shared_ptr<FsmTransition> tr2 = make_shared<FsmTransition>(u0, u1, make_shared<FsmLabel>(0, 1, pl));
	u0->addTransition(tr2);
	shared_ptr<FsmTransition> tr3 = make_shared<FsmTransition>(u1, u1, make_shared<FsmLabel>(1, 1, pl));
	u1->addTransition(tr3);
	shared_ptr<FsmTransition> tr4 = make_shared<FsmTransition>(u0, u2, make_shared<FsmLabel>(1, 1, pl));
	u0->addTransition(tr4);

	cout << ioEquivalenceCheck(q0, u0) << endl;
	}

	for (int i = 0; i < 30; i++) {
		auto fsm = Fsm::createRandomFsm("M1", 4, 4, 10, make_shared<FsmPresentationLayer>());
		cout << "fsm.size: " << fsm->size() << endl;
		auto fsm2 = fsm->minimise();
		cout << "fsm2.size: " << fsm2.size() << endl;
		cout << ioEquivalenceCheck(fsm->getInitialState(), fsm2.getInitialState()) << endl;
	}

	cout << "-------------------------------" << endl;

	for (int i = 0; i < 5; i++) {
		auto fsm = Fsm::createRandomFsm("M1", 4, 4, 10, make_shared<FsmPresentationLayer>());
		cout << "fsm.size: " << fsm->size() << endl;
		auto fsm2 = fsm->minimise();
		cout << "fsm2.size: " << fsm2.size() << endl;
		if (hasEquivalentStates(fsm2)) {
			cout << "FAULT" << endl;
		}
	}

	cout << "-------------------------------" << endl;

	{
		shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		shared_ptr<FsmNode> q0 = make_shared<FsmNode>(0, pl);
		shared_ptr<FsmNode> q1 = make_shared<FsmNode>(1, pl);

		shared_ptr<FsmTransition> tr0 = make_shared<FsmTransition>(q0, q1, make_shared<FsmLabel>(0, 1, pl));
		q0->addTransition(tr0);
		shared_ptr<FsmTransition> tr1 = make_shared<FsmTransition>(q1, q1, make_shared<FsmLabel>(1, 1, pl));
		q1->addTransition(tr1);
		shared_ptr<FsmTransition> tr5 = make_shared<FsmTransition>(q0, q0, make_shared<FsmLabel>(1, 1, pl));
		q0->addTransition(tr5);

		shared_ptr<FsmNode> u0 = make_shared<FsmNode>(0, pl);
		shared_ptr<FsmNode> u1 = make_shared<FsmNode>(1, pl);
		shared_ptr<FsmNode> u2 = make_shared<FsmNode>(2, pl);

		shared_ptr<FsmTransition> tr2 = make_shared<FsmTransition>(u0, u1, make_shared<FsmLabel>(0, 1, pl));
		u0->addTransition(tr2);
		shared_ptr<FsmTransition> tr3 = make_shared<FsmTransition>(u1, u1, make_shared<FsmLabel>(1, 1, pl));
		u1->addTransition(tr3);
		shared_ptr<FsmTransition> tr4 = make_shared<FsmTransition>(u0, u2, make_shared<FsmLabel>(1, 1, pl));
		u0->addTransition(tr4);

		cout << ioEquivalenceCheck(q0, u0) << endl;
	}

	cout << "-------------------------------" << endl;

	{
		shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		shared_ptr<FsmNode> q0 = make_shared<FsmNode>(0, pl);
		shared_ptr<FsmNode> q1 = make_shared<FsmNode>(1, pl);
		shared_ptr<FsmNode> q2 = make_shared<FsmNode>(2, pl);
		shared_ptr<FsmNode> q3 = make_shared<FsmNode>(3, pl);

		shared_ptr<FsmTransition> tr0 = make_shared<FsmTransition>(q0, q1, make_shared<FsmLabel>(1, 1, pl));
		q0->addTransition(tr0);
		shared_ptr<FsmTransition> tr1 = make_shared<FsmTransition>(q0, q2, make_shared<FsmLabel>(1, 1, pl));
		q0->addTransition(tr1);
		shared_ptr<FsmTransition> tr2 = make_shared<FsmTransition>(q2, q1, make_shared<FsmLabel>(2, 2, pl));
		q2->addTransition(tr2);
		shared_ptr<FsmTransition> tr3 = make_shared<FsmTransition>(q1, q3, make_shared<FsmLabel>(0, 1, pl));
		q1->addTransition(tr3);
		shared_ptr<FsmTransition> tr4 = make_shared<FsmTransition>(q3, q0, make_shared<FsmLabel>(0, 0, pl));
		q3->addTransition(tr4);

		shared_ptr<FsmNode> u0 = make_shared<FsmNode>(0, pl);
		shared_ptr<FsmNode> u1 = make_shared<FsmNode>(1, pl);
		shared_ptr<FsmNode> u2 = make_shared<FsmNode>(2, pl);
		shared_ptr<FsmNode> u3 = make_shared<FsmNode>(3, pl);

		shared_ptr<FsmTransition> tr5 = make_shared<FsmTransition>(u0, u1, make_shared<FsmLabel>(1, 1, pl));
		u0->addTransition(tr5);
		shared_ptr<FsmTransition> tr6 = make_shared<FsmTransition>(u1, u2, make_shared<FsmLabel>(2, 2, pl));
		u1->addTransition(tr6);
		shared_ptr<FsmTransition> tr7 = make_shared<FsmTransition>(u1, u3, make_shared<FsmLabel>(0, 1, pl));
		u1->addTransition(tr7);
		shared_ptr<FsmTransition> tr8 = make_shared<FsmTransition>(u2, u3, make_shared<FsmLabel>(0, 1, pl));
		u2->addTransition(tr8);
		shared_ptr<FsmTransition> tr9 = make_shared<FsmTransition>(u3, u0, make_shared<FsmLabel>(0, 0, pl));
		u3->addTransition(tr9);
		shared_ptr<FsmTransition> tr10 = make_shared<FsmTransition>(u2, u1, make_shared<FsmLabel>(0, 1, pl));
		u2->addTransition(tr10);
		

		cout << ioEquivalenceCheck(q0, u0) << endl;
	}

}

/*
	This function is used to test the checkForEqualStructure function
*/
void testCheckForEqualStructure() {
	cout << "testCheckForEqualStructure" << endl;

	cout << "positive cases:" << endl;
	for (int i = 0; i < 10; ++i) {
		auto fsm = Fsm::createRandomFsm("M1", 4, 4, 10, make_shared<FsmPresentationLayer>());

		cout << checkForEqualStructure(*fsm, *fsm) << endl;

		Fsm copy = Fsm(*fsm);
		cout << checkForEqualStructure(*fsm, copy) << endl;
		Fsm ofsm = fsm->transformToObservableFSM();

		cout << checkForEqualStructure(*fsm, copy) << endl;

		Fsm copy2 = Fsm(ofsm);
		Fsm minOfsm = ofsm.minimiseObservableFSM();
		cout << checkForEqualStructure(copy2, ofsm) << endl;

		cout << "-----------------------------------" << endl;
	}

	cout << "negative cases:" << endl;

	for (int i = 0; i < 10; ++i) {
		auto fsm = Fsm::createRandomFsm("M1", 4, 4, 10, make_shared<FsmPresentationLayer>());

		auto mutant = fsm->createMutant("mutant", 1, 1);
		
		cout << checkForEqualStructure(*fsm, *mutant) << endl;

		cout << "-----------------------------------" << endl;
	}

	auto fsm = Fsm::createRandomFsm("M1", 4, 4, 10, make_shared<FsmPresentationLayer>());

}

/*
	This function is used to test the getReachableStates function
*/
void testGetReachableStates() {
	for (int i = 0; i < 10; ++i) {
		auto fsm = Fsm::createRandomFsm("M1", 4, 4, 10, make_shared<FsmPresentationLayer>());

		unordered_set<shared_ptr<FsmNode>> reachable = getReachableStates(*fsm);

		vector<shared_ptr<FsmNode>> nodes = fsm->getNodes();
		unordered_set<shared_ptr<FsmNode>> nodeSet(nodes.begin(), nodes.end());

		fsmlib_assert("TC", reachable == nodeSet, "getReachableStates returns set containing each reachable state");
	}
	
	{
		shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		shared_ptr<FsmNode> q0 = make_shared<FsmNode>(0, pl);
		shared_ptr<FsmNode> q1 = make_shared<FsmNode>(1, pl);
		shared_ptr<FsmNode> q2 = make_shared<FsmNode>(2, pl);
		shared_ptr<FsmNode> q3 = make_shared<FsmNode>(3, pl);

		shared_ptr<FsmTransition> tr0 = make_shared<FsmTransition>(q0, q1, make_shared<FsmLabel>(1, 1, pl));
		q0->addTransition(tr0);
		shared_ptr<FsmTransition> tr1 = make_shared<FsmTransition>(q0, q2, make_shared<FsmLabel>(1, 1, pl));
		q0->addTransition(tr1);
		shared_ptr<FsmTransition> tr2 = make_shared<FsmTransition>(q2, q1, make_shared<FsmLabel>(2, 2, pl));
		q2->addTransition(tr2);
		//shared_ptr<FsmTransition> tr3 = make_shared<FsmTransition>(q1, q3, make_shared<FsmLabel>(0, 1, pl));
		//q1->addTransition(tr3);
		/*shared_ptr<FsmTransition> tr4 = make_shared<FsmTransition>(q3, q0, make_shared<FsmLabel>(0, 0, pl));
		q3->addTransition(tr4);*/

		Fsm fsm("M",2,2,{q0,q1,q2,q3},pl);
		unordered_set<shared_ptr<FsmNode>> reachable = getReachableStates(fsm);

		vector<shared_ptr<FsmNode>> nodes = fsm.getNodes();
		unordered_set<shared_ptr<FsmNode>> nodeSet(nodes.begin(), nodes.end());

		fsmlib_assert("TC", reachable != nodeSet, "getReachableStates returns set containing only reachable state");
	}
}



int main(int argc, char** argv)
{
	std::cout << "test start" << std::endl;
	//testIOEquivalenceCheck();
	//testCheckForEqualStructure();
	testGetReachableStates();
    
#if 0
    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
    test8();
    test9();
    test10();
    test10b();
    test11();
    test13();
    test14();
    test15();

    faux();

    
    gdc_test1();
    
    

    wVersusT();

    if ( argc < 6 ) {
        cerr << endl <<
        "Missing file names - exit." << endl;
        exit(1);
    }
    
    
    
    string fsmName(argv[1]);
    string fsmFile(argv[2]);
    
    /*
    string inputFile(argv[3]);
    string outputFile(argv[4]);
    string stateFile(argv[5]);
    */
    
    /* Create the presentation layer */
    //shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>(inputFile,outputFile,stateFile);
    
    shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
    
    /* Create an Fsm instance, using the transition relation file,
     * the presentation layer, and the FSM name
     */
    shared_ptr<Fsm> fsm = make_shared<Fsm>(fsmFile,pl,fsmName);
    
    /* Produce a GraphViz (.dot) representation of the created FSM */
    fsm->toDot(fsmName);
    
    /* Transform the FSM into an equivalent observable one */
    Fsm fsmObs = fsm->transformToObservableFSM();
    
    /* Output the observable FSM to a GraphViz file (.dot-file) */
    fsmObs.toDot(fsmObs.getName());
    
#endif
	//testTreeNodeAddConstInt1();
	//testTreeNodeAddConstInt2();
	//testTreeNodeAddConstInt3();
	//testTreeNodeEqualOperator1();
	//testTreeNodeEqualOperator2();
	//testTreeNodeCalcLeaves();
	//testTreeNodeClone();
	//testTreeNodeGetPath();
	//testTreeNodeSuperTreeOf1();
	//testTreeNodeSuperTreeOf2();
	//testTreeNodeTraverse();
	//testTreeNodeDeleteNode();
	//testTreeNodeDeleteSingleNode();
	//testAddToThisNode();
	//testTreeNodeAddIOListContainer();
	//testTreeNodeTentativeAddToThisNode();
	//testTreeNodeAfter();
	//testTreeNodeAddToThisNodeIOListContainer();

	//testTreeRemove();
	//testTreeToDot();
	//testTreeGetPrefixRelationTree();
	//testTreeTentativeAddToRoot();

	//testFsmPresentationLayerFileConstructor();
	//testFsmPresentationLayerDumpIn();
	//testFsmPresentationLayerComparePositive();
	//testFsmPresentationLayerCompareNegative();
	
	//testTraceEquals1Positive();
	//testTraceEquals1Negative();
	//testTraceEquals2Positive();
	//testTraceEquals2Negative();
	//testTraceOutputOperator();

	//testInputTraceOutputOperator();
	
	//testOutputTraceOutputOperator();

	//testOutputTreeContainsNegative();
	//testOutputTreeContainsPositive();
	//testOutputTreeToDot();
	//testOutputTreeGetOutputTraces();
	//testOutputTreeOutputOperator();

	//testTestSuiteIsEquivalentToPositive();
	//testTestSuiteIsEquivalentToNegative();
	//testTestSuiteIsReductionOfNegative();
	//testTestSuiteIsReductionOfPositive();

	//testIOListContainerConstructor();

	//testTraceSegmentGetCopy();

	//testSegmentedTraceEqualOperatorPositive();
	//testSegmentedTraceEqualOperatorNegative();

	//testHittingSetConstructor();
	//testHittingSetCalcMinCardHittingSet();

	//testHsTreeNodeIsHittingSetPositive();
	//testHsTreeNodeIsHittingSetNegative();
	//testHsTreeNodeExpandNode();
	//testHsTreeNodeToDot();

	//testInt2IntMapConstructor();

	//testPkTableRowIsEquivalentPositive();
	//testPkTableRowIsEquivalentNegative();

	//testPkTableMaxClassId();
	//testPkTableGetPkPlusOneTable();
	//testPkTableGetMembers();
	//testPkTableToFsm();

	//testDFSMTableGetP1Table();

	//testOFSMTableRowConstructor();
	//testOFSMTableIoEqualsPositive();
	//testOFSMTableIoEqualsNegative();
	//testOFSMTableClassEqualsPositive();
	//testOFSMTableClassEqualsNegative();

	//testOFSMTableConstructor();
	//testOFSMTableMaxClassId();
	//testOFSMTableNext();
	//testOFSMTableToFsm();

	//testFsmLabelOperatorLessThan();

	//testFsmNodeAddTransition();
	//testFsmNodeApply();
	//testFsmNodeAfter1();
	//testFsmNodeAfter2();
	//testFsmNodeGetDFSMTableRow();
	//testFsmNodeDistinguishedPositive();
	//testFsmNodeDistinguishedNegative();
	//testFsmNodeCalcDistinguishingTrace1();
	//testFsmNodeCalcDistinguishingTrace2();
	//testFsmNodeIsObservable();
	//testFsmNodeIsDeterministic();

    //testFsmAccept();
	//testFsmDeepCopyConstructor();
	//testFsmConstructor1();
	//testFsmConstructor2();
	//testFsmCreateRandomFsm();
	//testFsmCreateMutant();
	//testFsmDumpFsm();


	/*testMinimise();
	testWMethod();*/
	//testCharacterisationSet();
	//testGetDistTraces();
	//testHMethod();
	//testWpMethodWithDfsm(); 
	//testIntersectionCharacteristics();
    //test1();
    //test2();
    //test3();
    //test4();
    //test5();
    //test6();
    //test7();
    //test8();
    //test9();
    //test10();
    //test10b();
    //test11();
    //test13();
    //test14();
    //test15();
    exit(0);
    
}



