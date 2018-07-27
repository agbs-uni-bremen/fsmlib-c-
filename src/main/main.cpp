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

#include <algorithm>


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

bool checkDistinguishingCond(Dfsm &minimized) {
	const std::vector<shared_ptr<FsmNode>> nodes = minimized.getNodes();
	for (size_t i = 0; i < nodes.size() - 1; ++i) {
		for (size_t j = i + 1; j < nodes.size(); ++j) {
			if (!minimized.distinguishable(*nodes[i], *nodes[j])) {
				return false;
			}
		}
	}
	return true;
}


void testMinimise() {
	cout << "TC-DFSM-0017 Show that Dfsm::minimise() produces an "
		<< "equivalent minimal FSM"
		<< endl;

	auto pl = make_shared<FsmPresentationLayer>();
	auto dfsm = make_shared<Dfsm>("DFSM", 50, 5, 5, pl);
	Dfsm minimized = dfsm->minimise();
	std::vector<shared_ptr<FsmNode>> unreachableNodes;

	// check for unreachable nodes
	fsmlib_assert("TC-DFSM-0017",
		not minimized.removeUnreachableNodes(unreachableNodes),
		"Minimized Dfsm doesn't contain unreachable nodes");

	// check if states are distinguishable
	fsmlib_assert("TC-DFSM-0017",
		checkDistinguishingCond(minimized),
		"Each node pair of the minimized Dfsm is distinguishable");

	// check language equality
	fsmlib_assert("TC-DFSM-0017",
		minimized.intersect(*dfsm).isCompletelyDefined(),
		"Language of minimized Dfsm equals language of unminimized Dfsm");
}

void testWMethod() {
	cout << "TC-DFSM-0018 Show that Dfsm implModel only passes W-Method Testsuite "
		<< "if intersection is completely defined"
		<< endl;

	auto pl = make_shared<FsmPresentationLayer>();
	auto refModel = make_shared<Dfsm>("refModel", 50, 5, 5, pl);
	auto implModel = make_shared<Dfsm>("implModel", 50, 5, 5, pl);
	IOListContainer iolc = refModel->wMethod(0);

	// check language equality with W-Method Testsuite
	bool equal = true;
	for (auto trc : *(iolc.getIOLists())) {
		shared_ptr<InputTrace> iTr =
			make_shared<InputTrace>(trc, pl);
		if (not implModel->pass(refModel->applyDet(*iTr))) {
			equal = false;
			break;
		}
	}

	fsmlib_assert("TC-DFSM-0018",
		refModel->intersect(*implModel).isCompletelyDefined() == equal,
		"implModel passes W-Method Testsuite if and only if intersection is completely defined");
}

// Checks if tr1 is a prefix of tr2.
bool isPrefix(const std::vector<int> &tr1, const std::vector<int> &tr2) {
	if (tr1.size() > tr2.size()) {
		return false;
	}
	for (size_t i = 0; i < tr1.size(); ++i) {
		if (tr1[i] != tr2[i]) {
			return false;
		}
	}
	return true;
}

//Checks if ot1 is part of ot2. This is only the case if the InputTrace of ot1 is a prefix
//of the InputTrace of ot2 and if every OutputTrace of ot1 is a prefix of an OutputTrace
//of ot2.
bool containsOuputTree(OutputTree &ot1, OutputTree &ot2) {
	InputTrace it1 = ot1.getInputTrace();
	InputTrace it2 = ot2.getInputTrace();
	if (not isPrefix(it1.get(), it2.get())) {
		return false;
	}

	for (OutputTrace &outTr1 : ot1.getOutputTraces()) {
		bool prefix(false);
		for (OutputTrace &outTr2 : ot2.getOutputTraces()) {
			if (isPrefix(outTr1.get(), outTr2.get())) {
				prefix = true;
				break;
			}
		}
		if (not prefix) {
			return false;
		}
	}
	return true;
}

//void testIntersect() {
//	cout << "TC-DFSM-0019 Show that Fsm::intersect() produces FSM which accepts intersection "
//		<< "of the languages from the original FSMs"
//		<< endl;
//
//	auto pl = make_shared<FsmPresentationLayer>();
//	auto m1 = make_shared<Dfsm>("refModel", 3, 3, 3, pl)->minimise();
//	auto m2 = m1.createMutant("mutant", 2, 2)->minimise();
//	Fsm intersection = m1.intersect(m2).minimise();
//
//	std::cout << "m1.isDeterministic(): " << m1.isDeterministic() << std::endl;
//	std::cout << "m1.isCompletelyDefined(): " << m1.isCompletelyDefined() << std::endl;
//
//	std::cout << "m2.isDeterministic(): " << m2.isDeterministic() << std::endl;
//	std::cout << "m2.isCompletelyDefined(): " << m2.isCompletelyDefined() << std::endl;
//
//	std::cout << "intersection.isDeterministic(): " << intersection.isDeterministic() << std::endl;
//	std::cout << "intersection.isCompletelyDefined(): " << intersection.isCompletelyDefined() << std::endl;
//
//	std::cout << "m1.size(): " << m1.size();
//	std::cout << "intersection.size(): " << intersection.size();
//
//	// show that every trace of length n+m-1 in language of intersection is in language of m1 and m2 
//	IOListContainer iolc = IOListContainer(m1.getMaxInput(),
//			1,
//			intersection.size() + m1.size() - 1,
//			pl);
//
//	std::cout << "iolc created" << std::endl;
//
//	int c = 0;
//	for (auto trc : *(iolc.getIOLists())) {
//		if (c++ >= 10) break;
//		shared_ptr<InputTrace> iTr =
//			make_shared<InputTrace>(trc, pl);
//		OutputTree ot1 = intersection.apply(*iTr);
//		OutputTree ot2 = m1.apply(*iTr);
//		OutputTree ot3 = m2.apply(*iTr);		
//
//		std::cout << "ot2.contains(ot1): " << ot2.contains(ot1) << std::endl;
//		std::cout << "ot3.contains(ot1): " << ot3.contains(ot1) << std::endl;
//
//		std::cout << "containsOuputTree(ot1, ot2)" << containsOuputTree(ot1, ot2) << std::endl;
//		std::cout << "containsOuputTree(ot1, ot3)" << containsOuputTree(ot1, ot3) << std::endl;
//
//		std::cout << "intersection tr:\t" << ot1 << std::endl;
//		std::cout << std::endl;
//		std::cout << "ot2:\t\t\t" << ot2 << std::endl;
//		std::cout << std::endl;
//		std::cout << "ot3:\t\t\t" << ot3 << std::endl;
//	}
//}

void testIntersectionCharacteristics() {
	auto pl = make_shared<FsmPresentationLayer>();
	auto m1 = make_shared<Dfsm>("m1", 10, 3, 3, pl)->minimise();
	auto m2 = m1.createMutant("m2", 2, 2);

	fsmlib_assert("TC-DFSM-0019b",
		m1.intersect(*m2).isDeterministic(),
		"m1 or m2 deterministic => product automata deterministic");

	auto m3 = Fsm::createRandomFsm("m3", 3, 3, 3, pl);
	auto m4 = Fsm::createRandomFsm("m4", 3, 3, 3, pl);
	Fsm intersection = m3->intersect(*m4);
	if (not intersection.isDeterministic()) {
		fsmlib_assert("TC-DFSM-0019b",
			(not m3->isDeterministic()) and (not m4->isDeterministic()),
			"product automata of m3 and m4 nondeterministic => m3 and m4 nondeterministic");
	}
	if (intersection.isCompletelyDefined()) {
		fsmlib_assert("TC-DFSM-0019b",
			m3->isCompletelyDefined() and m4->isCompletelyDefined(),
			"product automata of m3 and m4 completely specified => m3 and m4 completely specified");
	}

	// TODO: m1 or m2 incomplete specified => product automata incomplete specified
}

bool equalSetOfOutputTrees(std::vector<OutputTree> &otv1, std::vector<OutputTree> &otv2) {
	if (otv1.size() != otv2.size()) {
		return false;
	}
	for (size_t i = 0; i < otv1.size(); ++i) {
		if (otv1[i] != otv2[i]) {
			return false;
		}
	}
	return true;
}

void testCharacterisationSet() {
	cout << "TC-FSM-0019 Show that calculated characterisation set "
		<< "distinguishes each pair of FSM states"
		<< endl;
	auto pl = make_shared<FsmPresentationLayer>();
	auto m1 = Fsm::createRandomFsm("M1", 3, 3, 10, pl)->minimise();
	IOListContainer iolc = m1.getCharacterisationSet();

	// calculate output trees for every node
	std::vector<std::vector<OutputTree>> outputTrees;
	for (const auto &node : m1.getNodes()) {		
		std::vector<OutputTree> traces;
		for (const auto trc : *(iolc.getIOLists())) {
			shared_ptr<InputTrace> iTr =
				make_shared<InputTrace>(trc, pl);
			traces.push_back(node->apply(*iTr));
		}
		outputTrees.push_back(traces);
	}

	// check if vector contains equal sets of output trees
	for (size_t i = 0; i < m1.getNodes().size() - 1; ++i) {
		for (size_t j = i + 1; j < m1.getNodes().size(); ++j) {
			if (equalSetOfOutputTrees(outputTrees[i], outputTrees[j])) {
				std::cout << "============= FAIL ==============" << std::endl;
			}
		}
	}
	std::cout << "============= PASS ============" << std::endl;
}

bool checkDistTracesForEachNodePair(Dfsm &m) {
	m.calculateDistMatrix();
	for (size_t i = 0; i < m.size() - 1; ++i) {
		for (size_t j = i + 1; j < m.size(); ++j) {
			auto ni = m.getNodes().at(i);
			auto nj = m.getNodes().at(j);
			auto distTraces = m.getDistTraces(*ni, *nj);
			for (auto trc : distTraces) {
				shared_ptr<InputTrace> iTr =
					make_shared<InputTrace>(*trc, m.getPresentationLayer());
				OutputTree oti = ni->apply(*iTr);
				OutputTree otj = nj->apply(*iTr);
				if (oti == otj) {
					return false;
				}
			}
		}
	}
	return true;
}

void testGetDistTraces() {
	cout << "TC-DFSM-0020 Show that calculated distinguishing traces "
		<< "in fact distinguish states"
		<< endl;

	auto pl = make_shared<FsmPresentationLayer>();
	auto m = make_shared<Dfsm>("M", 50, 5, 5, pl);

	fsmlib_assert("TC-DFSM-0020",
		checkDistTracesForEachNodePair(*m),
		"Each calculated distinguishing trace produces unequal set of output traces");

}

void testHMethod() {
	cout << "TC-DFSM-0021 Show that Dfsm implModel only passes H-Method Testsuite "
		<< "if intersection is completely defined"
		<< endl;

	auto pl = make_shared<FsmPresentationLayer>();
	auto refModel = make_shared<Dfsm>("refModel", 50, 5, 5, pl)->minimise();
	Fsm implModel = refModel.createMutant("mutant", 2, 2)->minimise();

	// refModel and implModel have to be compl. specified, deterministic and minimal
	// implModel should have at most the same size as refModel
	IOListContainer iolc = refModel.hMethodOnMinimisedDfsm(0);
	TestSuite ts1 = refModel.createTestSuite(iolc);
	TestSuite ts2 = implModel.createTestSuite(iolc);

	fsmlib_assert("TC-DFSM-0021",
		refModel.intersect(implModel).isCompletelyDefined() == ts1.isEquivalentTo(ts2),
		"implModel passes H-Method Testsuite if and only if intersection is completely defined");
}

void testWpMethodWithDfsm() {
	cout << "TC-DFSM-0022 Show that Dfsm implModel only passes Wp-Method Testsuite "
		<< "if intersection is completely defined"
		<< endl;

	auto pl = make_shared<FsmPresentationLayer>();
	auto refModel = make_shared<Dfsm>("refModel", 50, 5, 5, pl)->minimise();
	Fsm implModel = refModel.createMutant("mutant", 1, 1)->minimise();

	// refModel required to be minimised and observable
	// implModel required to have at most 0 additional states compared to refModel (both are prime machines)
	IOListContainer iolc = refModel.wpMethodOnMinimisedDfsm(0);
	TestSuite ts1 = refModel.createTestSuite(iolc);
	TestSuite ts2 = implModel.createTestSuite(iolc);

	// refModel and implModel required to be deterministic and completely specified
	fsmlib_assert("TC-DFSM-0022",
		refModel.intersect(implModel).isCompletelyDefined() == ts1.isEquivalentTo(ts2),
		"implModel passes Wp-Method Testsuite if and only if intersection is completely defined");
}


//===================================== TreeNode Tests ===================================================

// tests TreeNode::add(const int x). Checks if new TreeEdge is created for given input.
void testTreeNodeAddConstInt1(){
	int io = 1;
	shared_ptr<TreeNode> n1 = make_shared<TreeNode>();
	shared_ptr<TreeNode> ref = n1->add(io);
	fsmlib_assert("TC-TreeNode-NNNN",
		static_cast<shared_ptr<TreeNode>>(ref->getParent()) == n1,
		"parent of new node is old node");

	bool containedInChildren = false;
	for (shared_ptr<TreeEdge> e : *(n1->getChildren())) {
		if (e->getIO() == io && e->getTarget() == ref) {
			containedInChildren = true;
		}
	}
	fsmlib_assert("TC-TreeNode-NNNN",
		containedInChildren,
		"after call to TreeNode::add(x) there has to be a child labeled with x");
}

// tests TreeNode::add(const int x). Checks if no new TreeEdge is created if TreeEdge with matching io label
// already exists.
void testTreeNodeAddConstInt2() {
	int io = 1;
	shared_ptr<TreeNode> n1 = make_shared<TreeNode>();
	shared_ptr<TreeNode> child1 = n1->add(io);
	int oldNumChilds = n1->getChildren()->size();
	shared_ptr<TreeNode> child2 = n1->add(io);
	int newNumChilds = n1->getChildren()->size();
	fsmlib_assert("TC-TreeNode-NNNN",
		child2 == child1,
		"TreeNode::add(x) returns reference to target node of existing TreeEdge with matching io");
	fsmlib_assert("TC-TreeNode-NNNN",
		oldNumChilds == newNumChilds,
		"TreeNode::add(x) doesn't add new TreeEdge if TreeEdge with matching io already exists");
}

// tests TreeNode::add(const int x). Checks if new TreeEdge is created for given input. TreeNode already has children, but none with matching
// io label.
void testTreeNodeAddConstInt3() {
	shared_ptr<TreeNode> n1 = make_shared<TreeNode>();
	shared_ptr<TreeNode> child1 = n1->add(1);
	shared_ptr<TreeNode> child2 = n1->add(2);
	fsmlib_assert("TC-TreeNode-NNNN",
		child1 != child2,
		"calling TreeNode::add(x) and TreeNode::add(y) with x != y returns two different nodes");


	fsmlib_assert("TC-TreeNode-NNNN",
		n1->getChildren()->size() == 2,
		"number of TreeEdges contained in children attribute matches number of actually added values");
}

// tests addToThisNode(const std::vector<int> &lst) ( add(std::vector<int>::const_iterator lstIte, const std::vector<int>::const_iterator end) respectivly)
void testAddToThisNode() {
	shared_ptr<TreeNode> root = make_shared<TreeNode>();
	//shared_ptr<TreeNode> copy = root->clone();
	std::vector<int> inputs = {};
	root->addToThisNode(inputs);
	fsmlib_assert("TC-TreeNode-NNNN",
		root->isLeaf(),
		"addToThisNode() doesn't change tree if vector is empty");

	// root is leaf. input vector contains only one element
	inputs = { 1 };
	root->addToThisNode(inputs);
	fsmlib_assert("TC-TreeNode-NNNN",
		root->getChildren()->size() == 1
		&& root->getChildren()->at(0)->getIO() == 1
		&& root->getChildren()->at(0)->getTarget()->isLeaf(),
		"addToThisNode({1}) called on root (leaf) adds only one edge labeled by correct input. Target node is leaf");

	// root is no leaf (one edge labeled with 1; child is leaf). input vector contains two elements and shares no prefix with 
	// path starting at root.
	inputs = { 2,3 };
	shared_ptr<TreeEdge> e1 = make_shared<TreeEdge>(1, make_shared<TreeNode>());
	shared_ptr<TreeEdge> e2 = make_shared<TreeEdge>(2, make_shared<TreeNode>());
	shared_ptr<TreeEdge> e3 = make_shared<TreeEdge>(3, make_shared<TreeNode>());
	root->addToThisNode(inputs);
	fsmlib_assert("TC-TreeNode-NNNN",
		root->getChildren()->size() == 2
		&& root->hasEdge(e1) != nullptr
		&& root->hasEdge(e2) != nullptr
		&& root->hasEdge(e2)->getTarget()->getChildren()->size() == 1
		&& root->hasEdge(e2)->getTarget()->hasEdge(e3) != nullptr
		&& root->hasEdge(e2)->getTarget()->hasEdge(e3)->getTarget()->isLeaf(),
		"addToThisNode({2,3}) called on root (non leaf) adds only new edge to root labeled by 2. Target node gets edge with label 3 targeting leaf node");

	// root has one edge labeled with 1. child is leaf. vector contains two elements and shares prefix with path starting at root
	root = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(1, make_shared<TreeNode>()));
	inputs = { 1,2 };
	root->addToThisNode(inputs);
	fsmlib_assert("TC-TreeNode-NNNN",
		root->getChildren()->size() == 1
		&& root->getChildren()->at(0)->getIO() == 1
		&& root->getChildren()->at(0)->getTarget()->getChildren()->size() == 1
		&& root->getChildren()->at(0)->getTarget()->getChildren()->at(0)->getIO() == 2
		&& root->getChildren()->at(0)->getTarget()->getChildren()->at(0)->getTarget()->isLeaf(),
		"addToThisNode() called with vector sharing prefix with path starting at root doesn't add new edges for this prefix. "
		"result contains path represented by vector.");

	// root has two edges (1-edge, 2-edge). target of 1-edge has one edge (2-edge). vector equals path contained in this tree. 
	root = make_shared<TreeNode>();
	shared_ptr<TreeNode> child1 = make_shared<TreeNode>();
	shared_ptr<TreeNode> child2 = make_shared<TreeNode>();
	shared_ptr<TreeNode> grandChild = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(1, child1));
	root->add(make_shared<TreeEdge>(2, child2));
	child1->add(make_shared<TreeEdge>(2, grandChild));
	shared_ptr<TreeNode> copy = root->clone();
	inputs = { 1,2 };
	root->addToThisNode(inputs);
	fsmlib_assert("TC-TreeNode-NNNN",
		(*root == *copy),
		"addToThisNode() doesn't change tree if vector equals path contained in tree emanating from root");

	// root is leaf. vector contains two elements
	root = make_shared<TreeNode>();
	inputs = { 1,2 };
	root->addToThisNode(inputs);
	fsmlib_assert("TC-TreeNode-NNNN",
		root->getChildren()->size() == 1
		&& root->getChildren()->at(0)->getIO() == inputs.at(0)
		&& root->getChildren()->at(0)->getTarget()->getChildren()->size() == 1
		&& root->getChildren()->at(0)->getTarget()->getChildren()->at(0)->getIO() == inputs.at(1)
		&& root->getChildren()->at(0)->getTarget()->getChildren()->at(0)->getTarget()->isLeaf(),
		"addToThisNode() called on leaf adds path represented by vector.");
}

// helper prints vector<int>
void printVector(vector<int> & vec) {
	std::cout << "[";
	for (int i : vec) {
		std::cout << i << " ";
	}
	std::cout << "]\n";
}

// helper to print vector<vector<int>>
void printVectors(shared_ptr<vector<vector<int>>> vectors) {
	for (vector<int> &vec : *vectors) {
		printVector(vec);
	}
	std::cout << "\n";
}

// checks if every path contained in cloneIoll extended by every path in ioLstPtr is contained in rootIoll.
bool containsExpectedPaths(shared_ptr<vector<vector<int>>> cloneIoll, shared_ptr<vector<vector<int>>> rootIoll, shared_ptr<std::vector<std::vector<int>>> iolLstPtr) {
	printVectors(cloneIoll);
	printVectors(rootIoll);
	printVectors(iolLstPtr);
	for (vector<int> &clonePath : *cloneIoll) {
		for (vector<int> &extendingPath : *iolLstPtr) {
			vector<int> expectedPath(clonePath);
			std::cout << " expected vorher:";
			printVector(expectedPath);
			expectedPath.insert(expectedPath.cend(), extendingPath.cbegin(), extendingPath.cend());
			std::cout << " expected nachher:";
			printVector(expectedPath);
			if (find(rootIoll->cbegin(), rootIoll->cend(), expectedPath) == rootIoll->cend()) {
				return false;
			}
		}
	}
	return true;
}

// checks if every path of leaves is a result of a concatenation of a path from cloneIoll and a path from iolLstPtr.
bool containsNoUnexpectedPath(shared_ptr<vector<vector<int>>> cloneIoll, std::vector<std::shared_ptr<TreeNode>>& leaves, shared_ptr<std::vector<std::vector<int>>> iolLstPtr) {
	std::cout << "============================================================================" << std::endl;
	printVectors(cloneIoll);
	printVectors(iolLstPtr);

	// first build all possible concatenations and save them
	vector<vector<int>> concatenations;
	for (vector<int> &clonePath : *cloneIoll) {
		for (vector<int> &extendingPath : *iolLstPtr) {
			std::vector<int> concatenation(clonePath);
			concatenation.insert(concatenation.cend(), extendingPath.cbegin(), extendingPath.cend());
			concatenations.push_back(concatenation);
		}
	}

	std::cout << "concatenations: " << std::endl;
	printVectors(make_shared<vector<vector<int>>>(concatenations));

	// now check if every path from leaves is contained in concatenations
	for (shared_ptr<TreeNode> leave : leaves) {
		std::vector<int> path = leave->getPath();
		std::cout << "path: ";
		printVector(path);
		if (find(concatenations.cbegin(), concatenations.cend(), path) == concatenations.cend()) {
			return false;
		}
	}

	std::cout << "============================================================================" << std::endl;
	return true;
}

// tests TreeNode::add(const IOListContainer & tcl)
void testTreeNodeAddIOListContainer() {
	// root is leaf. iolc contains only one trace which is empty.
	shared_ptr<TreeNode> root = make_shared<TreeNode>();
	shared_ptr<TreeNode> clone = root->clone();
	std::vector<int> ioTrace1 = {};
	std::vector<std::vector<int>> ioLst = { ioTrace1 };
	shared_ptr<std::vector<std::vector<int>>> iolLstPtr = make_shared < std::vector<std::vector<int>>>(ioLst);
	shared_ptr<FsmPresentationLayer> presentationLayer = make_shared<FsmPresentationLayer>();	
	root->add(IOListContainer(iolLstPtr, presentationLayer));
	fsmlib_assert("TC-TreeNode-NNNN",
		root->superTreeOf(clone)
		&& (*root == *clone),
		"add(IOListContainer &iolc) called on leaf doesn't change tree if iolc only contains empty traces");

	// root is leaf. iolc contains only one trace which consists only of one input.
	root = make_shared<TreeNode>();
	clone = root->clone();
	ioTrace1 = { 1 };
	ioLst = { ioTrace1 };
	iolLstPtr = make_shared < std::vector<std::vector<int>>>(ioLst);
	presentationLayer = make_shared<FsmPresentationLayer>();

	shared_ptr<vector<vector<int>>> cloneIoll = make_shared<vector<vector<int>>>();
	std::vector<int> cloneV;
	clone->traverse(cloneV, cloneIoll);

	root->add(IOListContainer(iolLstPtr, presentationLayer));
	shared_ptr<vector<vector<int>>> rootIoll = make_shared<vector<vector<int>>>();
	std::vector<int> rootV;
	root->traverse(rootV, rootIoll);
	vector<shared_ptr<TreeNode>> leaves;
	root->calcLeaves(leaves);

	fsmlib_assert("TC-TreeNode-NNNN",
		root->superTreeOf(clone)
		&& containsExpectedPaths(cloneIoll, rootIoll, iolLstPtr), //containsNoUnexpectedPath(cloneIoll, leaves, iolLstPtr)
		"add(IOListContainer &iolc) result is super tree of old tree and result contains the expected paths");
	fsmlib_assert("TC-TreeNode-NNNN",
		containsNoUnexpectedPath(cloneIoll, leaves, iolLstPtr),
		"add(IOListContainer &iolc) result contains only expected paths");

	// root is leaf. iolc contains two traces. Each trace contains only one element (differing)
	root = make_shared<TreeNode>();
	clone = root->clone();
	ioTrace1 = { 1 };
	std::vector<int> ioTrace2;
	ioTrace2 = { 2 };
	ioLst = { ioTrace1, ioTrace2 };
	iolLstPtr = make_shared < std::vector<std::vector<int>>>(ioLst);
	presentationLayer = make_shared<FsmPresentationLayer>();

	cloneIoll = make_shared<vector<vector<int>>>();
	cloneV.clear();
	clone->traverse(cloneV, cloneIoll);

	root->add(IOListContainer(iolLstPtr, presentationLayer));
	rootIoll = make_shared<vector<vector<int>>>();
	rootV.clear();
	root->traverse(rootV, rootIoll);
	leaves.clear();
	root->calcLeaves(leaves);

	fsmlib_assert("TC-TreeNode-NNNN",
		root->superTreeOf(clone)
		&& containsExpectedPaths(cloneIoll, rootIoll, iolLstPtr),
		"add(IOListContainer &iolc) result is super tree of old tree and result contains the expected paths");
	fsmlib_assert("TC-TreeNode-NNNN",
		containsNoUnexpectedPath(cloneIoll, leaves, iolLstPtr),
		"add(IOListContainer &iolc) result contains only expected paths");


	// root is leaf. iolc contains two paths. first path consists of two elements. second path consists of one element.
	root = make_shared<TreeNode>();
	clone = root->clone();
	ioTrace1 = { 1,2 };
	ioTrace2 = { 2 };
	ioLst = { ioTrace1, ioTrace2 };
	iolLstPtr = make_shared < std::vector<std::vector<int>>>(ioLst);
	presentationLayer = make_shared<FsmPresentationLayer>();

	cloneIoll = make_shared<vector<vector<int>>>();
	cloneV.clear();
	clone->traverse(cloneV, cloneIoll);

	root->add(IOListContainer(iolLstPtr, presentationLayer));
	rootIoll = make_shared<vector<vector<int>>>();
	rootV.clear();
	root->traverse(rootV, rootIoll);
	leaves.clear();
	root->calcLeaves(leaves);

	fsmlib_assert("TC-TreeNode-NNNN",
		root->superTreeOf(clone)
		&& containsExpectedPaths(cloneIoll, rootIoll, iolLstPtr),
		"add(IOListContainer &iolc) result is super tree of old tree and result contains the expected paths");
	fsmlib_assert("TC-TreeNode-NNNN",
		containsNoUnexpectedPath(cloneIoll, leaves, iolLstPtr),
		"add(IOListContainer &iolc) result contains only expected paths");

	//// root is leaf. iolc contains two paths. first path consists of two elements. second path consists of one element.
	//root = make_shared<TreeNode>();
	//clone = root->clone();
	//ioTrace1 = { 1,2 };
	//ioTrace2 = { 1 };
	//ioLst = { ioTrace1, ioTrace2 };
	//iolLstPtr = make_shared < std::vector<std::vector<int>>>(ioLst);
	//presentationLayer = make_shared<FsmPresentationLayer>();
	//
	//cloneIoll = make_shared<vector<vector<int>>>();
	//cloneV.clear();
	//clone->traverse(cloneV, cloneIoll);
	//
	//root->add(IOListContainer(iolLstPtr, presentationLayer));
	//rootIoll = make_shared<vector<vector<int>>>();
	//rootV.clear();
	//root->traverse(rootV, rootIoll);
	//leaves.clear();
	//root->calcLeaves(leaves);
    //
	//assert("TC-TreeNode-NNNN",
	//	root->superTreeOf(clone)
	//	&& containsExpectedPaths(cloneIoll, rootIoll, iolLstPtr),
	//	"add(IOListContainer &iolc) result is super tree of old tree and result contains the expected paths");

	// root has two childs (leaves). iolc contains one paths, which doesn't share a prefix with a path already contained in tree.
	root = make_shared<TreeNode>();
	shared_ptr<TreeNode> child1 = make_shared<TreeNode>();
	shared_ptr<TreeNode> child2 = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(1, child1));
	root->add(make_shared<TreeEdge>(2, child2));
	clone = root->clone();
	ioTrace1 = { 3 };
	ioLst = { ioTrace1 };
	iolLstPtr = make_shared < std::vector<std::vector<int>>>(ioLst);
	presentationLayer = make_shared<FsmPresentationLayer>();

	cloneIoll = make_shared<vector<vector<int>>>();
	cloneV.clear();
	clone->traverse(cloneV, cloneIoll);

	root->add(IOListContainer(iolLstPtr, presentationLayer));
	rootIoll = make_shared<vector<vector<int>>>();
	rootV.clear();
	root->traverse(rootV, rootIoll);
	leaves.clear();
	root->calcLeaves(leaves);

	fsmlib_assert("TC-TreeNode-NNNN",
		root->superTreeOf(clone)
		&& containsExpectedPaths(cloneIoll, rootIoll, iolLstPtr),
		"add(IOListContainer &iolc) result is super tree of old tree and result contains the expected paths");
	fsmlib_assert("TC-TreeNode-NNNN",
		containsNoUnexpectedPath(cloneIoll, leaves, iolLstPtr),
		"add(IOListContainer &iolc) result contains only expected paths");

	// root has two childs (leaves). iolc contains one path, which is equals to a path already contained in tree.
	root = make_shared<TreeNode>();
	child1 = make_shared<TreeNode>();
	child2 = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(1, child1));
	root->add(make_shared<TreeEdge>(2, child2));
	clone = root->clone();
	ioTrace1 = { 1 };
	ioLst = { ioTrace1 };
	iolLstPtr = make_shared < std::vector<std::vector<int>>>(ioLst);
	presentationLayer = make_shared<FsmPresentationLayer>();

	cloneIoll = make_shared<vector<vector<int>>>();
	cloneV.clear();
	clone->traverse(cloneV, cloneIoll);

	root->add(IOListContainer(iolLstPtr, presentationLayer));
	rootIoll = make_shared<vector<vector<int>>>();
	rootV.clear();
	root->traverse(rootV, rootIoll);
	leaves.clear();
	root->calcLeaves(leaves);

	fsmlib_assert("TC-TreeNode-NNNN",
		root->superTreeOf(clone)
		&& containsExpectedPaths(cloneIoll, rootIoll, iolLstPtr),
		"add(IOListContainer &iolc) result is super tree of old tree and result contains the expected paths");
	fsmlib_assert("TC-TreeNode-NNNN",
		containsNoUnexpectedPath(cloneIoll, leaves, iolLstPtr),
		"add(IOListContainer &iolc) result contains only expected paths");

	// root has two childs (leaves). iolc contains one path, which has a prefix already contained in tree.
	root = make_shared<TreeNode>();
	child1 = make_shared<TreeNode>();
	child2 = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(1, child1));
	root->add(make_shared<TreeEdge>(2, child2));
	clone = root->clone();
	ioTrace1 = { 1, 2 };
	ioLst = { ioTrace1 };
	iolLstPtr = make_shared < std::vector<std::vector<int>>>(ioLst);
	presentationLayer = make_shared<FsmPresentationLayer>();

	cloneIoll = make_shared<vector<vector<int>>>();
	cloneV.clear();
	clone->traverse(cloneV, cloneIoll);

	root->add(IOListContainer(iolLstPtr, presentationLayer));
	rootIoll = make_shared<vector<vector<int>>>();
	rootV.clear();
	root->traverse(rootV, rootIoll);
	leaves.clear();
	root->calcLeaves(leaves);

	fsmlib_assert("TC-TreeNode-NNNN",
		root->superTreeOf(clone)
		&& containsExpectedPaths(cloneIoll, rootIoll, iolLstPtr),
		"add(IOListContainer &iolc) result is super tree of old tree and result contains the expected paths");
	fsmlib_assert("TC-TreeNode-NNNN",
		containsNoUnexpectedPath(cloneIoll, leaves, iolLstPtr),
		"add(IOListContainer &iolc) result contains only expected paths");
}

// tests operator==(TreeNode const & treeNode1, TreeNode const & treeNode2)  (positive case)
void testTreeNodeEqualOperator1() {
	shared_ptr<TreeNode> n1 = make_shared<TreeNode>();
	shared_ptr<TreeNode> n2 = make_shared<TreeNode>();
	fsmlib_assert("TC-TreeNode-NNNN",
		*n1 == *n2,
		"operator== returns true if both nodes are equal");

	shared_ptr<TreeNode> n11 = make_shared<TreeNode>();
	shared_ptr<TreeNode> n21 = make_shared<TreeNode>();
	n1->add(make_shared<TreeEdge>(1, n11));
	n2->add(make_shared<TreeEdge>(1, n21));
	fsmlib_assert("TC-TreeNode-NNNN",
		*n1 == *n2,
		"operator== returns true if both nodes are equal");

	shared_ptr<TreeNode> n12 = make_shared<TreeNode>();
	shared_ptr<TreeNode> n22 = make_shared<TreeNode>();
	n1->add(make_shared<TreeEdge>(2, n12));
	n2->add(make_shared<TreeEdge>(2, n22));
	fsmlib_assert("TC-TreeNode-NNNN",
		*n1 == *n2,
		"operator== returns true if both nodes are equal");

	shared_ptr<TreeNode> n111 = make_shared<TreeNode>();
	shared_ptr<TreeNode> n112 = make_shared<TreeNode>();
	n11->add(make_shared<TreeEdge>(1, n111));
	n11->add(make_shared<TreeEdge>(2, n112));
	shared_ptr<TreeNode> n211 = make_shared<TreeNode>();	
	shared_ptr<TreeNode> n212 = make_shared<TreeNode>();
	n21->add(make_shared<TreeEdge>(1, n211));
	n21->add(make_shared<TreeEdge>(2, n212));

	fsmlib_assert("TC-TreeNode-NNNN",
		*n1 == *n2,
		"operator== returns true if both nodes are equal");
}

// tests operator==(TreeNode const & treeNode1, TreeNode const & treeNode2)  (negative case)
void testTreeNodeEqualOperator2() {
	shared_ptr<TreeNode> n1 = make_shared<TreeNode>();
	shared_ptr<TreeNode> n2 = make_shared<TreeNode>();
	n1->deleteSingleNode();
	fsmlib_assert("TC-TreeNode-NNNN",
		!(*n1 == *n2),
		"operator== returns false if only one of the TreeNode instances is marked as deleted");

	n1 = make_shared<TreeNode>();
	n2 = make_shared<TreeNode>();
	n2->add(make_shared<TreeEdge>(1, make_shared<TreeNode>()));
	fsmlib_assert("TC-TreeNode-NNNN",
		!(*n1 == *n2),
		"operator== returns false if the compared TreeNode instances have different number of children");

	n1->add(make_shared<TreeEdge>(2, make_shared<TreeNode>()));
	fsmlib_assert("TC-TreeNode-NNNN",
		!(*n1 == *n2) && n1->getChildren()->size() == n2->getChildren()->size(),
		"operator== returns false if both TreeNode instances have same number of children but edges are labeled differently");

	n1 = make_shared<TreeNode>();
	n2 = make_shared<TreeNode>();
	shared_ptr<TreeNode> n11 = make_shared<TreeNode>();
	shared_ptr<TreeNode> n21 = make_shared<TreeNode>();
	n1->add(make_shared<TreeEdge>(1, n11));
	n2->add(make_shared<TreeEdge>(1, n21));
	n11->add(make_shared<TreeEdge>(1, make_shared<TreeNode>()));
	fsmlib_assert("TC-TreeNode-NNNN",
		!(*n1 == *n2) && n11->getChildren()->size() != n21->getChildren()->size(),
		"operator== returns false if two corresponding childs of both TreeNode instances differ in the number of children");

	n21->add(make_shared<TreeEdge>(2, make_shared<TreeNode>()));
	fsmlib_assert("TC-TreeNode-NNNN",
		!(*n1 == *n2) && n11->getChildren()->size() == n21->getChildren()->size(),
		"operator== returns false if two corresponding childs of both TreeNode instances differ in the labeling of their children");

	n11->add(make_shared<TreeEdge>(2, make_shared<TreeNode>()));
	n21->add(make_shared<TreeEdge>(1, make_shared<TreeNode>()));
	n11->deleteSingleNode();
	fsmlib_assert("TC-TreeNode-NNNN",
		!(*n1 == *n2),
		"operator== returns false if two corresponding childs differ in beeing marked as deleted");

	n21->deleteSingleNode();
	fsmlib_assert("TC-TreeNode-NNNN",
		(*n1 == *n2),
		"operator== returns true if both instances are equal");
}

// tests TreeNode::calcLeaves(vector<shared_ptr<TreeNode const>>& leaves) const
void testTreeNodeCalcLeaves() {
	shared_ptr<TreeNode> root = make_shared<TreeNode>();
	std::vector<shared_ptr<TreeNode>> leaves;
	root->calcLeaves(leaves);
	fsmlib_assert("TC-TreeNode-NNNN",
		leaves.size() == 1 && leaves[0] == root,
		"calcLeaves() called on leave adds this leave");

	leaves = std::vector<shared_ptr<TreeNode>>();
	shared_ptr<TreeNode> child1 = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(1, child1));
	root->calcLeaves(leaves);
	fsmlib_assert("TC-TreeNode-NNNN",
		leaves.size() == 1 && leaves[0] == child1,
		"calcLeaves() called on parent with leave-child adds this leave-child");

	leaves = std::vector<shared_ptr<TreeNode>>();
	shared_ptr<TreeNode> child2 = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(2, child2));
	root->calcLeaves(leaves);
	fsmlib_assert("TC-TreeNode-NNNN",
		leaves.size() == 2 && std::find(leaves.cbegin(), leaves.cend(), child1) != leaves.cend() 
		&& std::find(leaves.cbegin(), leaves.cend(), child2) != leaves.cend(),
		"calcLeaves() called on parent with leave-childs adds all leave-childs");

	leaves = std::vector<shared_ptr<TreeNode>>();
	shared_ptr<TreeNode> grandChild1 = make_shared<TreeNode>();
	child1->add(make_shared<TreeEdge>(1, grandChild1));
	root->calcLeaves(leaves);
	fsmlib_assert("TC-TreeNode-NNNN",
		leaves.size() == 2 && std::find(leaves.cbegin(), leaves.cend(), grandChild1) != leaves.cend()
		&& std::find(leaves.cbegin(), leaves.cend(), child2) != leaves.cend(),
		"calcLeaves() called on parent with leave-child and leave-grandchild adds leave-child and leave-grandchild");

	leaves = std::vector<shared_ptr<TreeNode>>();
	shared_ptr<TreeNode> grandChild2 = make_shared<TreeNode>();
	child2->add(make_shared<TreeEdge>(1, grandChild2));
	root->calcLeaves(leaves);
	fsmlib_assert("TC-TreeNode-NNNN",
		leaves.size() == 2 && std::find(leaves.cbegin(), leaves.cend(), grandChild1) != leaves.cend()
		&& std::find(leaves.cbegin(), leaves.cend(), grandChild2) != leaves.cend(),
		"calcLeaves() called on root with two leave-grandchilds adds both leave-grandchilds");

	leaves = std::vector<shared_ptr<TreeNode>>();
	shared_ptr<TreeNode> grandChild3 = make_shared<TreeNode>();
	child2->add(make_shared<TreeEdge>(3, grandChild3));
	root->calcLeaves(leaves);
	fsmlib_assert("TC-TreeNode-NNNN",
		leaves.size() == 3 && std::find(leaves.cbegin(), leaves.cend(), grandChild1) != leaves.cend()
		&& std::find(leaves.cbegin(), leaves.cend(), grandChild2) != leaves.cend()
		&& std::find(leaves.cbegin(), leaves.cend(), grandChild3) != leaves.cend(),
		"calcLeaves() called on root with three leave-grandchilds adds these leave-grandchilds");
}

// extracts all TreeNodes reachable from given node.
void extractAllTreeNodes(shared_ptr<TreeNode> node, std::vector<shared_ptr<TreeNode>> & nodes) {
	nodes.push_back(node);
	for (const auto &edge : *(node->getChildren())) {
		extractAllTreeNodes(edge->getTarget(), nodes);
	}
}

// extracts all TreeEdges reachable from given node.
void extractAllTreeEdges(shared_ptr<TreeNode> node, std::vector<shared_ptr<TreeEdge>> & edges) {
	for (const auto &edge : *(node->getChildren())) {
		edges.push_back(edge);
		extractAllTreeEdges(edge->getTarget(), edges);
	}
}

// only called with TreeNodes which are known to be equal (*original == *copy).
// checks if no TreeNode and no TreeEdge is contained in both trees.
bool isDeepCopyOfEqualNode(shared_ptr<TreeNode> original, shared_ptr<TreeNode> copy) {
	std::vector<shared_ptr<TreeNode>> originalNodes;
	extractAllTreeNodes(original, originalNodes);
	std::vector<shared_ptr<TreeNode>> copyNodes;
	std::cout << "originalNodes.size(): " << originalNodes.size() << std::endl;
	
	extractAllTreeNodes(copy, copyNodes);
	std::cout << "copyNodes.size(): " << copyNodes.size() << std::endl;
	for (shared_ptr<TreeNode> node : originalNodes) {
		for (shared_ptr<TreeNode> cnode : copyNodes) {
			if (node == cnode) {
				return false;
			}
		}
	}

	//std::vector<shared_ptr<TreeEdge>> originalEdges;
	//extractAllTreeEdges(original, originalEdges);
	//std::vector<shared_ptr<TreeEdge>> copyEdges;
	//extractAllTreeEdges(copy, copyEdges);
	//for (shared_ptr<TreeEdge> edge : originalEdges) {
	//	for (shared_ptr<TreeEdge> cedge : copyEdges) {
	//		if (edge == cedge) {
	//			return false;
	//		}
	//	}
	//}
}

// tests TreeNode::clone() const
void testTreeNodeClone() {
	shared_ptr<TreeNode> root = make_shared<TreeNode>();
	shared_ptr<TreeNode> clone = root->clone();
	fsmlib_assert("TC-TreeNode-NNNN",
		(*root == *clone) && isDeepCopyOfEqualNode(root, clone),    //(root != clone),
		"clone equals original and clone is deep copy");

	shared_ptr<TreeNode> child1 = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(1, child1));
	clone = root->clone();
	fsmlib_assert("TC-TreeNode-NNNN",
		(*root == *clone) && isDeepCopyOfEqualNode(root, clone),
		"clone equals original and clone is deep copy");

	shared_ptr<TreeNode> child2 = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(2, child2));
	clone = root->clone();
	fsmlib_assert("TC-TreeNode-NNNN",
		(*root == *clone) && isDeepCopyOfEqualNode(root, clone),
		"clone equals original and clone is deep copy");

	shared_ptr<TreeNode> grandChild1 = make_shared<TreeNode>();
	child1->add(make_shared<TreeEdge>(1, grandChild1));
	clone = root->clone();
	fsmlib_assert("TC-TreeNode-NNNN",
		(*root == *clone) && isDeepCopyOfEqualNode(root, clone),
		"clone equals original and clone is deep copy");

	shared_ptr<TreeNode> grandChild2 = make_shared<TreeNode>();
	child1->add(make_shared<TreeEdge>(2, grandChild2));
	clone = root->clone();
	fsmlib_assert("TC-TreeNode-NNNN",
		(*root == *clone) && isDeepCopyOfEqualNode(root, clone),
		"clone equals original and clone is deep copy");
}

// tests TreeNode::getPath() const
void testTreeNodeGetPath() {
	shared_ptr<TreeNode> root = make_shared<TreeNode>();
	std::vector<int> expected = {};
	fsmlib_assert("TC-TreeNode-NNNN",
		expected == root->getPath(),
		"getPath invoked on root returns empty list");

	shared_ptr<TreeNode> child1 = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(1, child1));
	expected = {1};
	fsmlib_assert("TC-TreeNode-NNNN",
		expected == child1->getPath(),
		"getPath invoked on child returns list containing only the input needed to reach it");

	shared_ptr<TreeNode> child2 = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(2, child2));
	expected = { 2 };
	fsmlib_assert("TC-TreeNode-NNNN",
		expected == child2->getPath(),
		"getPath invoked on child returns list containing only the input needed to reach it");

	shared_ptr<TreeNode> grandChild1 = make_shared<TreeNode>();
	shared_ptr<TreeNode> grandChild2 = make_shared<TreeNode>();
	child1->add(make_shared<TreeEdge>(1, grandChild1));
	child1->add(make_shared<TreeEdge>(2, grandChild2));
	expected = { 1, 1 };
	fsmlib_assert("TC-TreeNode-NNNN",
		expected == grandChild1->getPath(),
		"getPath invoked on grandchild returns list containing only the two inputs needed to reach it");

	expected = { 1, 2 };
	fsmlib_assert("TC-TreeNode-NNNN",
		expected == grandChild2->getPath(),
		"getPath invoked on grandchild returns list containing only the two inputs needed to reach it");
}

// tests TreeNode::superTreeOf(const shared_ptr<TreeNode> otherNode) const.
// negative case
void testTreeNodeSuperTreeOf1() {
	shared_ptr<TreeNode> root = make_shared<TreeNode>();
	shared_ptr<TreeNode> rootOther = make_shared<TreeNode>();
	shared_ptr<TreeNode> childOther1 = make_shared<TreeNode>();
	rootOther->add(make_shared<TreeEdge>(1, childOther1));
	fsmlib_assert("TC-TreeNode-NNNN",
		!root->superTreeOf(rootOther),
		"superTreeOf() returns false if rootOther has more children than root");

	shared_ptr<TreeNode> childOther2 = make_shared<TreeNode>();
	rootOther->add(make_shared<TreeEdge>(2, childOther2));
	fsmlib_assert("TC-TreeNode-NNNN",
		!root->superTreeOf(rootOther),
		"superTreeOf() returns false if rootOther has more children than root");

	shared_ptr<TreeNode> child1 = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(1, child1));
	fsmlib_assert("TC-TreeNode-NNNN",
		!root->superTreeOf(rootOther),
		"superTreeOf() returns false if rootOther has more children than root");

	shared_ptr<TreeNode> grandChildOther1 = make_shared<TreeNode>();
	childOther2->add(make_shared<TreeEdge>(1, grandChildOther1));
	fsmlib_assert("TC-TreeNode-NNNN",
		!root->superTreeOf(rootOther),
		"superTreeOf() returns false if rootOther has more children than root");

	shared_ptr<TreeNode> child2 = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(2, child2));
	fsmlib_assert("TC-TreeNode-NNNN",
		!root->superTreeOf(rootOther),
		"superTreeOf() returns false if one corresponding child of root and rootOther has different number of childs");

	//-----------------------------------------------------------------------------------------------

	root = make_shared<TreeNode>();
	rootOther = make_shared<TreeNode>();
	child1 = make_shared<TreeNode>();
	childOther1 = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(1, child1));
	rootOther->add(make_shared<TreeEdge>(2, childOther1));
	fsmlib_assert("TC-TreeNode-NNNN",
		!root->superTreeOf(rootOther),
		"superTreeOf() returns false if rootOther has TreeEdge with label not existent in root");

	child2 = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(2, child2));
	childOther2 = make_shared<TreeNode>();
	rootOther->add(make_shared<TreeEdge>(3, childOther2));
	fsmlib_assert("TC-TreeNode-NNNN",
		!root->superTreeOf(rootOther),
		"superTreeOf() returns false if rootOther has TreeEdge with label not existent in root");
}

// tests TreeNode::superTreeOf(const shared_ptr<TreeNode> otherNode) const.
// positive case
void testTreeNodeSuperTreeOf2() {
	shared_ptr<TreeNode> root = make_shared<TreeNode>();
	shared_ptr<TreeNode> rootOther = make_shared<TreeNode>();
	fsmlib_assert("TC-TreeNode-NNNN",
		root->superTreeOf(rootOther),
		"superTreeOf() returns true if root and rootOther are equal");

	shared_ptr<TreeNode> child1 = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(1, child1));
	fsmlib_assert("TC-TreeNode-NNNN",
		root->superTreeOf(rootOther),
		"superTreeOf() returns true if root contains rootOther");

	shared_ptr<TreeNode> child2 = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(2, child2));
	shared_ptr<TreeNode> childOther1 = make_shared<TreeNode>();
	rootOther->add(make_shared<TreeEdge>(1, childOther1));
	fsmlib_assert("TC-TreeNode-NNNN",
		root->superTreeOf(rootOther),
		"superTreeOf() returns true if root contains rootOther");

	shared_ptr<TreeNode> childOther2 = make_shared<TreeNode>();
	rootOther->add(make_shared<TreeEdge>(2, childOther2));
	fsmlib_assert("TC-TreeNode-NNNN",
		root->superTreeOf(rootOther) && *root == *rootOther,
		"superTreeOf() returns true if root and rootOther are equal");

	shared_ptr<TreeNode> grandChild1 = make_shared<TreeNode>();
	child1->add(make_shared<TreeEdge>(1, grandChild1));
	fsmlib_assert("TC-TreeNode-NNNN",
		root->superTreeOf(rootOther),
		"superTreeOf() returns true if root contains rootOther");

	shared_ptr<TreeNode> grandChild2 = make_shared<TreeNode>();
	child1->add(make_shared<TreeEdge>(2, grandChild2));
	shared_ptr<TreeNode> grandChildOther1 = make_shared<TreeNode>();
	childOther1->add(make_shared<TreeEdge>(2, grandChildOther1));
	fsmlib_assert("TC-TreeNode-NNNN",
		root->superTreeOf(rootOther),
		"superTreeOf() returns true if root contains rootOther");
}

// tests TreeNode::traverse(vector<int>& v,shared_ptr<vector<vector<int>>> ioll)
void testTreeNodeTraverse() {
	shared_ptr<TreeNode> root = make_shared<TreeNode>();
	std::vector<int> v;
	std::shared_ptr<std::vector<std::vector<int>>> ioll = make_shared<std::vector<std::vector<int>>>();
	root->traverse(v, ioll);
	fsmlib_assert("TC-TreeNode-NNNN",
		v.size() == 0 && ioll->size() == 1 && ioll->at(0) == v,
		"traverse() called on leave doesn't add inputs to current input vector v but adds v to ioll");

	v.clear();
	ioll->clear();
	shared_ptr<TreeNode> child1 = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(1, child1));
	shared_ptr<TreeNode> child2 = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(2, child2));
	root->traverse(v, ioll);
	/*const std::vector<int> e1 = {};
	const std::vector<int> e2 = { 1 };
	const std::vector<int> e3 = { 2 };*/
	fsmlib_assert("TC-TreeNode-NNNN",
		ioll->size() == 3 && std::find(ioll->cbegin(), ioll->cend(), root->getPath()) != ioll->cend()
		&& std::find(ioll->cbegin(), ioll->cend(), child1->getPath()) != ioll->cend()
		&& std::find(ioll->cbegin(), ioll->cend(), child2->getPath()) != ioll->cend(),
		"traverse() called on node n adds all int paths from n to leaves of the tree");

	v.clear();
	ioll->clear();
	shared_ptr<TreeNode> grandChild1 = make_shared<TreeNode>();
	child1->add(make_shared<TreeEdge>(3, grandChild1));
	shared_ptr<TreeNode> grandChild2 = make_shared<TreeNode>();
	child1->add(make_shared<TreeEdge>(4, grandChild2));
	root->traverse(v, ioll);
	//const std::vector<int> e4 = { 1,3 };
	//const std::vector<int> e5 = { 1,4 };
	fsmlib_assert("TC-TreeNode-NNNN",
		ioll->size() == 5 && std::find(ioll->cbegin(), ioll->cend(), root->getPath()) != ioll->cend()
		&& std::find(ioll->cbegin(), ioll->cend(), child1->getPath()) != ioll->cend()
		&& std::find(ioll->cbegin(), ioll->cend(), child2->getPath()) != ioll->cend()
		&& std::find(ioll->cbegin(), ioll->cend(), grandChild1->getPath()) != ioll->cend()
		&& std::find(ioll->cbegin(), ioll->cend(), grandChild2->getPath()) != ioll->cend(),
		"traverse() called on node n adds all int paths from n to leaves of the tree");
}

// tests TreeNode::deleteNode()
void testTreeNodeDeleteNode() {
	shared_ptr<TreeNode> root = make_shared<TreeNode>();
	root->deleteNode();
	fsmlib_assert("TC-TreeNode-NNNN",
		root->isDeleted(),
		"deleteNode() called on root marks root as deleted");

	// parent (root) with one child (leaf). child gets deleted
	root = make_shared<TreeNode>();
	shared_ptr<TreeNode> child1 = make_shared<TreeNode>();
	shared_ptr<TreeEdge> rootToChild1 = make_shared<TreeEdge>(1, child1);
	root->add(rootToChild1);
	child1->deleteNode();
	fsmlib_assert("TC-TreeNode-NNNN",
		!(root->isDeleted())
		&& child1->isDeleted()
		&& std::find(root->getChildren()->cbegin(), root->getChildren()->cend(), rootToChild1) == root->getChildren()->cend(),
		"deleteNode() called on child marks child as deleted, removes child from children list of parent and doesn't mark parent as deleted");

	// parent (root) with one child (leaf). parent gets deleted
	root = make_shared<TreeNode>();
	child1 = make_shared<TreeNode>();
	rootToChild1 = make_shared<TreeEdge>(1, child1);
	root->add(rootToChild1);
	root->deleteNode();
	fsmlib_assert("TC-TreeNode-NNNN",
		root->isDeleted()
		&& !(child1->isDeleted())
		&& std::find(root->getChildren()->cbegin(), root->getChildren()->cend(), rootToChild1) != root->getChildren()->cend(),
		"deleteNode() called on parent of undeleted child marks parent as deleted but doesn't change children");

	// parent (root) with two children (leaves) child1 and child2. child2 gets deleted
	root = make_shared<TreeNode>();
	child1 = make_shared<TreeNode>();
	shared_ptr<TreeNode> child2 = make_shared<TreeNode>();
	rootToChild1 = make_shared<TreeEdge>(1, child1);
	shared_ptr<TreeEdge> rootToChild2 = make_shared<TreeEdge>(2, child2);
	root->add(rootToChild1);
	root->add(rootToChild2);
	child2->deleteNode();
	fsmlib_assert("TC-TreeNode-NNNN",
		!(root->isDeleted())
		&& !(child1->isDeleted())
		&& child2->isDeleted()
		&& std::find(root->getChildren()->cbegin(), root->getChildren()->cend(), rootToChild1) != root->getChildren()->cend()
		&& std::find(root->getChildren()->cbegin(), root->getChildren()->cend(), rootToChild2) == root->getChildren()->cend(),
		"deleteNode() called on child2 (leaf) marks child2 as deleted, removes it from child list of its parent and doesn't change the other child");

	// root has child1 as only child. child1 has grandChild1 as only child, which is a leaf. child1 gets deleted.
	root = make_shared<TreeNode>();
	child1 = make_shared<TreeNode>();
	shared_ptr<TreeNode> grandChild1 = make_shared<TreeNode>();
	rootToChild1 = make_shared<TreeEdge>(1, child1);
	shared_ptr<TreeEdge> child1ToGrandChild1 = make_shared<TreeEdge>(1, grandChild1);
	root->add(rootToChild1);
	child1->add(child1ToGrandChild1);
	child1->deleteNode();
	fsmlib_assert("TC-TreeNode-NNNN",
		!(root->isDeleted())
		&& child1->isDeleted()
		&& !(grandChild1->isDeleted())
		&& std::find(root->getChildren()->cbegin(), root->getChildren()->cend(), rootToChild1) != root->getChildren()->cend()
		&& std::find(child1->getChildren()->cbegin(), child1->getChildren()->cend(), child1ToGrandChild1) != child1->getChildren()->cend(),
		"deleteNode() called on child (non leaf) marks it as deleted but doesn't change parent or grandchilds of parent");
	
	// root has child1 as only child. child1 has grandChild1 as only child, which is a leaf. child1 is already deleted.
	// In the next step grandChild1 gets deleted
	grandChild1->deleteNode();
	fsmlib_assert("TC-TreeNode-NNNN",
		!(root->isDeleted())
		&& child1->isDeleted()
		&& grandChild1->isDeleted()
		&& std::find(root->getChildren()->cbegin(), root->getChildren()->cend(), rootToChild1) == root->getChildren()->cend()
		&& std::find(child1->getChildren()->cbegin(), child1->getChildren()->cend(), child1ToGrandChild1) == child1->getChildren()->cend(),
		"deleteNode() called on child (leaf) with already deleted parent marks this child as deleted, removes it from "
		"list of childs from its parent and removes parent from child list of parents parent");

	// root has child1 as only child. child1 has grandChild1 as only child, which is a leaf. grandChild1 gets deleted.
	root = make_shared<TreeNode>();
	child1 = make_shared<TreeNode>();
	grandChild1 = make_shared<TreeNode>();
	rootToChild1 = make_shared<TreeEdge>(1, child1);
	child1ToGrandChild1 = make_shared<TreeEdge>(1, grandChild1);
	root->add(rootToChild1);
	child1->add(child1ToGrandChild1);
	grandChild1->deleteNode();
	fsmlib_assert("TC-TreeNode-NNNN",
		!(root->isDeleted())
		&& !(child1->isDeleted())
		&& grandChild1->isDeleted()
		&& std::find(root->getChildren()->cbegin(), root->getChildren()->cend(), rootToChild1) != root->getChildren()->cend()
		&& std::find(child1->getChildren()->cbegin(), child1->getChildren()->cend(), child1ToGrandChild1) == child1->getChildren()->cend(),
		"deleteNode() called on child (leaf) with already non deleted parent marks this child as deleted, removes it from "
		"list of childs from its parent and doesn't delete parent");
}

// tests TreeNode::deleteSingleNode()
void testTreeNodeDeleteSingleNode() {
	shared_ptr<TreeNode> root = make_shared<TreeNode>();
	root->deleteSingleNode();
	fsmlib_assert("TC-TreeNode-NNNN",
		root->isDeleted(),
		"deleteSingleNode() called on root marks root as deleted");

	// parent (root) with one child (leaf). child gets deleted
	root = make_shared<TreeNode>();
	shared_ptr<TreeNode> child1 = make_shared<TreeNode>();
	shared_ptr<TreeEdge> rootToChild1 = make_shared<TreeEdge>(1, child1);
	root->add(rootToChild1);
	child1->deleteSingleNode();
	fsmlib_assert("TC-TreeNode-NNNN",
		!(root->isDeleted())
		&& child1->isDeleted()
		&& std::find(root->getChildren()->cbegin(), root->getChildren()->cend(), rootToChild1) == root->getChildren()->cend(),
		"deleteSingleNode() called on child marks child as deleted, removes child from children list of parent and doesn't mark parent as deleted");

	// parent (root) with one child (leaf). parent gets deleted
	root = make_shared<TreeNode>();
	child1 = make_shared<TreeNode>();
	rootToChild1 = make_shared<TreeEdge>(1, child1);
	root->add(rootToChild1);
	root->deleteSingleNode();
	fsmlib_assert("TC-TreeNode-NNNN",
		root->isDeleted()
		&& !(child1->isDeleted())
		&& std::find(root->getChildren()->cbegin(), root->getChildren()->cend(), rootToChild1) != root->getChildren()->cend(),
		"deleteSingleNode() called on parent of undeleted child marks parent as deleted but doesn't change children");

	// root has child1 as only child. child1 has grandChild1 as only child, which is a leaf. child1 gets deleted.
	root = make_shared<TreeNode>();
	child1 = make_shared<TreeNode>();
	shared_ptr<TreeNode> grandChild1 = make_shared<TreeNode>();
	rootToChild1 = make_shared<TreeEdge>(1, child1);
	shared_ptr<TreeEdge> child1ToGrandChild1 = make_shared<TreeEdge>(1, grandChild1);
	root->add(rootToChild1);
	child1->add(child1ToGrandChild1);
	child1->deleteSingleNode();
	fsmlib_assert("TC-TreeNode-NNNN",
		!(root->isDeleted())
		&& child1->isDeleted()
		&& !(grandChild1->isDeleted())
		&& std::find(root->getChildren()->cbegin(), root->getChildren()->cend(), rootToChild1) != root->getChildren()->cend()
		&& std::find(child1->getChildren()->cbegin(), child1->getChildren()->cend(), child1ToGrandChild1) != child1->getChildren()->cend(),
		"deleteSingleNode() called on child (non leaf) marks it as deleted but doesn't change parent or grandchilds of parent");

	// root has child1 as only child. child1 has grandChild1 as only child, which is a leaf. child1 is already deleted.
	// In the next step grandChild1 gets deleted
	grandChild1->deleteSingleNode();
	fsmlib_assert("TC-TreeNode-NNNN",
		!(root->isDeleted())
		&& child1->isDeleted()
		&& grandChild1->isDeleted()
		&& std::find(root->getChildren()->cbegin(), root->getChildren()->cend(), rootToChild1) != root->getChildren()->cend()
		&& std::find(child1->getChildren()->cbegin(), child1->getChildren()->cend(), child1ToGrandChild1) == child1->getChildren()->cend(),
		"deleteNode() called on child (leaf) with already deleted parent marks this child as deleted, removes it from "
		"list of childs of its parent but doesn't remove parent from child list of parents parent");
}

// tests TreeNode::tentativeAddToThisNode(std::vector<int>::const_iterator start, std::vector<int>::const_iterator stop, std::shared_ptr<TreeNode>& n)
void testTreeNodeTentativeAddToThisNode() {
	// case 1: path is already contained in tree -> returns 0

	// root is leaf and inpPath is empty
	shared_ptr<TreeNode> ref = make_shared<TreeNode>();
	shared_ptr<TreeNode> root = make_shared<TreeNode>();
	vector<int> inpPath = {};
	fsmlib_assert("TC-TreeNode-NNNN",
		root->tentativeAddToThisNode(inpPath.cbegin(), inpPath.cend(), ref) == 0
		&& ref == root,
		"tentativeAddToThisNode() returns 0 if path is already contained in tree");

	// root has two children (leaves). inpPath contains one element (already contained as a label)
	ref = make_shared<TreeNode>();
	root = make_shared<TreeNode>();
	shared_ptr<TreeNode> c1 = make_shared<TreeNode>();
	shared_ptr<TreeNode> c2 = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(1, c1));
	root->add(make_shared<TreeEdge>(2, c2));
	inpPath = { 1 };
	fsmlib_assert("TC-TreeNode-NNNN",
		root->tentativeAddToThisNode(inpPath.cbegin(), inpPath.cend(), ref) == 0
		&& ref == c1,
		"tentativeAddToThisNode() returns 0 if path is already contained in tree");

	// root has two childs (c1 and c2). c1 is a leaf. c2 has two childs (gc1 and gc2). Both are leaves. inpPath contains one element (already contained as a label)
	ref = make_shared<TreeNode>();
	root = make_shared<TreeNode>();
	c1 = make_shared<TreeNode>();
	c2 = make_shared<TreeNode>();
	shared_ptr<TreeNode> gc1 = make_shared<TreeNode>();
	shared_ptr<TreeNode> gc2 = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(1, c1));
	root->add(make_shared<TreeEdge>(2, c2));
	c2->add(make_shared<TreeEdge>(1, gc1));
	c2->add(make_shared<TreeEdge>(2, gc2));
	inpPath = { 2 };
	fsmlib_assert("TC-TreeNode-NNNN",
		root->tentativeAddToThisNode(inpPath.cbegin(), inpPath.cend(), ref) == 0
		&& ref == c2,
		"tentativeAddToThisNode() returns 0 if path is already contained in tree");

	// root has two childs (c1 and c2). c1 is a leaf. c2 has two childs (gc1 and gc2). Both are leaves. inpPath (2 elements) is already contained in tree.
	ref = make_shared<TreeNode>();
	root = make_shared<TreeNode>();
	c1 = make_shared<TreeNode>();
	c2 = make_shared<TreeNode>();
	gc1 = make_shared<TreeNode>();
	gc2 = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(1, c1));
	root->add(make_shared<TreeEdge>(2, c2));
	c2->add(make_shared<TreeEdge>(1, gc1));
	c2->add(make_shared<TreeEdge>(2, gc2));
	inpPath = { 2, 1 };
	fsmlib_assert("TC-TreeNode-NNNN",
		root->tentativeAddToThisNode(inpPath.cbegin(), inpPath.cend(), ref) == 0
		&& ref == gc1,
		"tentativeAddToThisNode() returns 0 if path is already contained in tree");

	// case 2: prefix of path reaches leaf of tree (no new branch needed) -> returns 1

	// root is leaf. path contains one element
	ref = make_shared<TreeNode>();
	root = make_shared<TreeNode>();
	inpPath = { 1 };
	fsmlib_assert("TC-TreeNode-NNNN",
		root->tentativeAddToThisNode(inpPath.cbegin(), inpPath.cend(), ref) == 1
		&& ref == root,
		"tentativeAddToThisNode() returns 1 if a prefix of the path reaches a leaf "
		"and adding the path would require to extend the tree");

	// root has two childs (c1 and c2). c1 and c2 are leaves. inpPath contains two elements.
	ref = make_shared<TreeNode>();
	root = make_shared<TreeNode>();
	c1 = make_shared<TreeNode>();
	c2 = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(1, c1));
	root->add(make_shared<TreeEdge>(2, c2));
	//std::cout << "root leaf:" << root->isLeaf() << std::endl;
	//std::cout << "root children size: " << root->getChildren()->size() << std::endl;
	//std::cout << "c1 children size: " << c1->getChildren()->size() << std::endl;
	//std::cout << "c2 children size: " << c2->getChildren()->size() << std::endl;
	inpPath = { 1,2 };
	fsmlib_assert("TC-TreeNode-NNNN",
		root->tentativeAddToThisNode(inpPath.cbegin(), inpPath.cend(), ref) == 1
		&& ref == c1,
		"tentativeAddToThisNode() returns 1 if a prefix of the path reaches a leaf "
		"and adding the path would require to extend the tree");

	// root has two childs (c1 and c2). c1 is a leaf. c2 has two childs (gc1 and gc2). Both are leaves. inpPath (3 elements) 
	ref = make_shared<TreeNode>();
	root = make_shared<TreeNode>();
	c1 = make_shared<TreeNode>();
	c2 = make_shared<TreeNode>();
	gc1 = make_shared<TreeNode>();
	gc2 = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(1, c1));
	root->add(make_shared<TreeEdge>(2, c2));
	c2->add(make_shared<TreeEdge>(1, gc1));
	c2->add(make_shared<TreeEdge>(2, gc2));
	inpPath = { 2, 1, 2 };

	fsmlib_assert("TC-TreeNode-NNNN",
		root->tentativeAddToThisNode(inpPath.cbegin(), inpPath.cend(), ref) == 1
		&& ref == gc1,
		"tentativeAddToThisNode() returns 1 if a prefix of the path reaches a leaf "
		"and adding the path would require to extend the tree");

	// case 3: path not fully contained and no prefix of path reaches leaf of tree (new branch needed) -> returns 2

	// root is has child c1. c1 is a leaf. path contains one element
	ref = make_shared<TreeNode>();
	root = make_shared<TreeNode>();
	c1 = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(1, c1));
	inpPath = { 2 };

	fsmlib_assert("TC-TreeNode-NNNN",
		root->tentativeAddToThisNode(inpPath.cbegin(), inpPath.cend(), ref) == 2
		&& ref == root,
		"tentativeAddToThisNode() returns 2 if no prefix of the path reaches a leaf "
		"and adding the path would require to create a new branch");

	// root is has childs c1 and c2. c1 is a leaf. c2 has two childs gc1 and gc2. gc1 and gc2 are leaves.
	// inpPath contains 2 elements.
	ref = make_shared<TreeNode>();
	root = make_shared<TreeNode>();
	c1 = make_shared<TreeNode>();
	c2 = make_shared<TreeNode>();
	gc1 = make_shared<TreeNode>();
	gc2 = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(1, c1));
	root->add(make_shared<TreeEdge>(2, c2));
	c2->add(make_shared<TreeEdge>(1, gc1));
	c2->add(make_shared<TreeEdge>(2, gc2));
	inpPath = { 2, 3 };

	fsmlib_assert("TC-TreeNode-NNNN",
		root->tentativeAddToThisNode(inpPath.cbegin(), inpPath.cend(), ref) == 2
		&& ref == c2,
		"tentativeAddToThisNode() returns 2 if no prefix of the path reaches a leaf "
		"and adding the path would require to create a new branch");

	// root is has childs c1 and c2. c1 is a leaf. c2 has two childs gc1 and gc2. gc1 and gc2 are leaves.
	// inpPath contains 3 elements.
	inpPath = { 2, 3, 2};
	fsmlib_assert("TC-TreeNode-NNNN",
		root->tentativeAddToThisNode(inpPath.cbegin(), inpPath.cend(), ref) == 2
		&& ref == c2,
		"tentativeAddToThisNode() returns 2 if no prefix of the path reaches a leaf "
		"and adding the path would require to create a new branch");
}

// tests TreeNode::after(std::vector<int>::const_iterator lstIte, const std::vector<int>::const_iterator end)
void testTreeNodeAfter() {
	// case 1: result is no nullptr

	// root is a leaf. path is empty.
	shared_ptr<TreeNode> root = make_shared<TreeNode>();
	vector<int> path = {};
	fsmlib_assert("TC-TreeNode-NNNN",
		root->after(path.cbegin(), path.cend()) == root,
		"after() called with empty path returns root");

	// root has two children (c1 and c2). path is empty
	shared_ptr<TreeNode> c1 = make_shared<TreeNode>();
	shared_ptr<TreeNode> c2 = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(1, c1));
	root->add(make_shared<TreeEdge>(2, c2));
	fsmlib_assert("TC-TreeNode-NNNN",
		root->after(path.cbegin(), path.cend()) == root,
		"after() called with empty path returns root");

	// root has two children (c1 and c2). c1 is a leaf. c2 has two children (gc1 and gc2). gc1 and gc2 are leaves.
	// path contains one element and is a prefix of a path which is contained in the tree.
	shared_ptr<TreeNode> gc1 = make_shared<TreeNode>();
	shared_ptr<TreeNode> gc2 = make_shared<TreeNode>();
	c2->add(make_shared<TreeEdge>(1, gc1));
	c2->add(make_shared<TreeEdge>(2, gc2));
	path = { 2 };
	fsmlib_assert("TC-TreeNode-NNNN",
		root->after(path.cbegin(), path.cend()) == c2,
		"after() called with non empty path which is a prefix of a contained path returns the node, which can be reached with this prefix");

	// root has two children (c1 and c2). c1 is a leaf. c2 has two children (gc1 and gc2). gc1 and gc2 are leaves.
	// path contains two elements and equals a path which is contained in the tree.
	path = { 2, 1 };
	fsmlib_assert("TC-TreeNode-NNNN",
		root->after(path.cbegin(), path.cend()) == gc1,
		"after() called with non empty path which equals a contained path returns the node, which can be reached with this path");


	// case 2: result is nullptr

	// root is leaf. path is not empty
	root = make_shared<TreeNode>();
	path = { 1 };
	fsmlib_assert("TC-TreeNode-NNNN",
		root->after(path.cbegin(), path.cend()) == nullptr,
		"after() called with path that can't be completely matched against tree returns nullptr.");

	// root has two children (c1 and c2). path contains one element which doesn't match any edge label.
	c1 = make_shared<TreeNode>();
	c2 = make_shared<TreeNode>();
	root->add(make_shared<TreeEdge>(1, c1));
	root->add(make_shared<TreeEdge>(2, c2));
	path = { 3 };
	fsmlib_assert("TC-TreeNode-NNNN",
		root->after(path.cbegin(), path.cend()) == nullptr,
		"after() called with path that can't be completely matched against tree returns nullptr.");

	// root has two children (c1 and c2). c1 and c2 are leaves. path contains two elements (path is longer than any contained path). 
	path = { 1,2 };
	fsmlib_assert("TC-TreeNode-NNNN",
		root->after(path.cbegin(), path.cend()) == nullptr,
		"after() called with path that can't be completely matched against tree returns nullptr.");

	// root has two children (c1 and c2). c1 is a leaf. c2 has two children (gc1 and gc2). gc1 and gc2 are leaves.
	// path contains two elements.
	gc1 = make_shared<TreeNode>();
	gc2 = make_shared<TreeNode>();
	c2->add(make_shared<TreeEdge>(1, gc1));
	c2->add(make_shared<TreeEdge>(2, gc2));
	path = { 2,3 };
	fsmlib_assert("TC-TreeNode-NNNN",
		root->after(path.cbegin(), path.cend()) == nullptr,
		"after() called with path that can't be completely matched against tree returns nullptr.");

	// root has two children (c1 and c2). c1 is a leaf. c2 has two children (gc1 and gc2). gc1 and gc2 are leaves.
	// path contains three elements (path is longer than any contained path).
	path = { 2, 1, 3 };
	fsmlib_assert("TC-TreeNode-NNNN",
		root->after(path.cbegin(), path.cend()) == nullptr,
		"after() called with path that can't be completely matched against tree returns nullptr.");
}

// checks if every path (from root to leaf) in newNode is a path in oldNode or in iolc.
// This is a condition the result of TreeNode::addToThisNode(const IOListContainer & tcl) has to fullfill
bool resultContainsOnlyExpectedPaths(shared_ptr<TreeNode> newNode, shared_ptr<TreeNode> oldNode, IOListContainer & iolc) {
	std::cout << "==========================================================" << std::endl;
	std::cout << "resultContainsOnlyExpectedPaths:" << std::endl;
	std::vector<shared_ptr<TreeNode>> newLeaves;
	newNode->calcLeaves(newLeaves);

	std::vector<shared_ptr<TreeNode>> oldLeaves;
	oldNode->calcLeaves(oldLeaves);

	// store all paths from oldNode (from root to a leaf) and from iolc in vector paths.
	std::vector<std::vector<int>> paths;
	for (shared_ptr<TreeNode> oldLeaf : oldLeaves) {
		paths.push_back(oldLeaf->getPath());
	}
	paths.insert(paths.cend(), iolc.getIOLists()->cbegin(), iolc.getIOLists()->cend());

	printVectors(make_shared<std::vector<std::vector<int>>>(paths));

	// check if every path in newNode is contained in paths
	for (shared_ptr<TreeNode> newLeaf : newLeaves) {
		std::vector<int> p = newLeaf->getPath();
		printVector(p);
		if (find(paths.cbegin(), paths.cend(), newLeaf->getPath()) == paths.cend()) {
			return false;
		}
	}
	std::cout << "==========================================================" << std::endl;
	return true;
}

// checks if newNode contains each path contained in iolc.
// This is a condition the result of TreeNode::addToThisNode(const IOListContainer & tcl) has to fullfill
bool resultContainsEachAddedPath(shared_ptr<TreeNode> newNode, IOListContainer & iolc) {	
	std::cout << "==========================================================" << std::endl;
	std::cout << "resultContainsEachAddedPath:" << std::endl;
	std::vector<shared_ptr<TreeNode>> reachable;
	extractAllTreeNodes(newNode, reachable);

	// store each path from newNode in vector paths
	std::vector<std::vector<int>> paths;
	for (shared_ptr<TreeNode> node : reachable) {
		paths.push_back(node->getPath());
	}
	printVectors(make_shared<std::vector<std::vector<int>>>(paths));

	// check if each path from iolc is contained in paths
	for (std::vector<int> path : *(iolc.getIOLists())) {
		if (find(paths.cbegin(), paths.cend(), path) == paths.cend()) {
			return false;
		}
	}
	std::cout << "==========================================================" << std::endl;
	return true;
}

// checks if each TreeNode in the tree emanating from newNode is observable. (There are no two TreeEdges emanating from the same TreeNode, which
// have the same label)
bool treeIsObservable(shared_ptr<TreeNode> newNode) {	
	std::vector<shared_ptr<TreeNode>> reachable;
	extractAllTreeNodes(newNode, reachable);

	// check the children of each node
	for (shared_ptr<TreeNode> node : reachable) {
		std::unordered_set<int> nodeLabels;
		for (shared_ptr<TreeEdge> edge : *(node->getChildren())) {
			// if label of edge couldn't be inserted, it is already contained, so there is another edge with same label
			if (!nodeLabels.insert(edge->getIO()).second) {
				return false;
			}
		}
	}
	return true;
}

// tests TreeNode::addToThisNode(const IOListContainer & tcl)
void testTreeNodeAddToThisNodeIOListContainer() {
	// root is a leaf. IOListContainer is empty.
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		shared_ptr<TreeNode> old = root->clone();
		std::vector<std::vector<int>> ioLst = { };
		shared_ptr<std::vector<std::vector<int>>> iolLstPtr = make_shared < std::vector<std::vector<int>>>(ioLst);
		shared_ptr<FsmPresentationLayer> presentationLayer = make_shared<FsmPresentationLayer>();
		IOListContainer iolc1(iolLstPtr, presentationLayer);
		root->addToThisNode(iolc1);
		fsmlib_assert("TC-TreeNode-NNNN",
			root->superTreeOf(old),
			"result of addToThisNode(const IOListContainer & tcl) is a super tree of the original TreeNode, so every path from original is still contained in result");
		fsmlib_assert("TC-TreeNode-NNNN",
			resultContainsOnlyExpectedPaths(root, old, iolc1),
			"result of addToThisNode(const IOListContainer & tcl) contains only expected paths (paths from original tree or added paths)");
		fsmlib_assert("TC-TreeNode-NNNN",
			resultContainsEachAddedPath(root, iolc1),
			"result of addToThisNode(const IOListContainer & tcl) contains each added path");
		fsmlib_assert("TC-TreeNode-NNNN",
			treeIsObservable(root),
			"result of addToThisNode(const IOListContainer & tcl) contains no redundant prefixes");
	}

	// root is a leaf. IOListContainer contains only empty vectors.
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		shared_ptr<TreeNode> old = root->clone();
		std::vector<int> inputs1 = {};
		std::vector<std::vector<int>> ioLst = {inputs1};
		shared_ptr<std::vector<std::vector<int>>> iolLstPtr = make_shared < std::vector<std::vector<int>>>(ioLst);
		shared_ptr<FsmPresentationLayer> presentationLayer = make_shared<FsmPresentationLayer>();
		IOListContainer iolc1(iolLstPtr, presentationLayer);
		root->addToThisNode(iolc1);
		fsmlib_assert("TC-TreeNode-NNNN",
			root->superTreeOf(old),
			"result of addToThisNode(const IOListContainer & tcl) is a super tree of the original TreeNode, so every path from original is still contained in result");
		fsmlib_assert("TC-TreeNode-NNNN",
			resultContainsOnlyExpectedPaths(root, old, iolc1),
			"result of addToThisNode(const IOListContainer & tcl) contains only expected paths (paths from original tree or added paths)");
		fsmlib_assert("TC-TreeNode-NNNN",
			resultContainsEachAddedPath(root, iolc1),
			"result of addToThisNode(const IOListContainer & tcl) contains each added path");
		fsmlib_assert("TC-TreeNode-NNNN",
			treeIsObservable(root),
			"result of addToThisNode(const IOListContainer & tcl) contains no redundant prefixes");
	}

	// root is a leaf. IOListContainer contains two paths ({1} and {2}).
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		shared_ptr<TreeNode> old = root->clone();
		std::vector<int> inputs1 = { 1 };
		std::vector<int> inputs2 = { 2 };
		std::vector<std::vector<int>> ioLst = { inputs1, inputs2 };
		shared_ptr<std::vector<std::vector<int>>> iolLstPtr = make_shared < std::vector<std::vector<int>>>(ioLst);
		shared_ptr<FsmPresentationLayer> presentationLayer = make_shared<FsmPresentationLayer>();
		IOListContainer iolc1(iolLstPtr, presentationLayer);
		root->addToThisNode(iolc1);
		fsmlib_assert("TC-TreeNode-NNNN",
			root->superTreeOf(old),
			"result of addToThisNode(const IOListContainer & tcl) is a super tree of the original TreeNode, so every path from original is still contained in result");
		fsmlib_assert("TC-TreeNode-NNNN",
			resultContainsOnlyExpectedPaths(root, old, iolc1),
			"result of addToThisNode(const IOListContainer & tcl) contains only expected paths (paths from original tree or added paths)");
		fsmlib_assert("TC-TreeNode-NNNN",
			resultContainsEachAddedPath(root, iolc1),
			"result of addToThisNode(const IOListContainer & tcl) contains each added path");
		fsmlib_assert("TC-TreeNode-NNNN",
			treeIsObservable(root),
			"result of addToThisNode(const IOListContainer & tcl) contains no redundant prefixes");
	}

	// root has one child (c1). c1 is a leaf. IOListContainer contains two paths ({1} and {2}). TreeEdge matches one contained path.
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		shared_ptr<TreeNode> c1 = make_shared<TreeNode>();
		root->add(make_shared<TreeEdge>(1, c1));
		shared_ptr<TreeNode> old = root->clone();
		std::vector<int> inputs1 = { 1 };
		std::vector<int> inputs2 = { 2 };
		std::vector<std::vector<int>> ioLst = { inputs1, inputs2 };
		shared_ptr<std::vector<std::vector<int>>> iolLstPtr = make_shared < std::vector<std::vector<int>>>(ioLst);
		shared_ptr<FsmPresentationLayer> presentationLayer = make_shared<FsmPresentationLayer>();
		IOListContainer iolc1(iolLstPtr, presentationLayer);
		root->addToThisNode(iolc1);
		fsmlib_assert("TC-TreeNode-NNNN",
			root->superTreeOf(old),
			"result of addToThisNode(const IOListContainer & tcl) is a super tree of the original TreeNode, so every path from original is still contained in result");
		fsmlib_assert("TC-TreeNode-NNNN",
			resultContainsOnlyExpectedPaths(root, old, iolc1),
			"result of addToThisNode(const IOListContainer & tcl) contains only expected paths (paths from original tree or added paths)");
		fsmlib_assert("TC-TreeNode-NNNN",
			resultContainsEachAddedPath(root, iolc1),
			"result of addToThisNode(const IOListContainer & tcl) contains each added path");
		fsmlib_assert("TC-TreeNode-NNNN",
			treeIsObservable(root),
			"result of addToThisNode(const IOListContainer & tcl) contains no redundant prefixes");
	}

	// root has two childs (c1 and c2). c2 is a leaf. c1 has two children (gc1 and gc2). gc1 and gc2 are leaves.
	// IOListContainer contains two paths ({1, 3} and {1, 3}). (both are equal and match a prefix already contained in tree)
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		shared_ptr<TreeNode> c1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> c2 = make_shared<TreeNode>();
		shared_ptr<TreeNode> gc1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> gc2 = make_shared<TreeNode>();
		root->add(make_shared<TreeEdge>(1, c1));
		root->add(make_shared<TreeEdge>(2, c2));
		c1->add(make_shared<TreeEdge>(1, gc1));
		c1->add(make_shared<TreeEdge>(2, gc2));
		shared_ptr<TreeNode> old = root->clone();
		std::vector<int> inputs1 = { 1, 3 };
		std::vector<int> inputs2 = { 1, 3 };
		std::vector<std::vector<int>> ioLst = { inputs1, inputs2 };
		shared_ptr<std::vector<std::vector<int>>> iolLstPtr = make_shared < std::vector<std::vector<int>>>(ioLst);
		shared_ptr<FsmPresentationLayer> presentationLayer = make_shared<FsmPresentationLayer>();
		IOListContainer iolc1(iolLstPtr, presentationLayer);
		root->addToThisNode(iolc1);
		fsmlib_assert("TC-TreeNode-NNNN",
			root->superTreeOf(old),
			"result of addToThisNode(const IOListContainer & tcl) is a super tree of the original TreeNode, so every path from original is still contained in result");
		fsmlib_assert("TC-TreeNode-NNNN",
			resultContainsOnlyExpectedPaths(root, old, iolc1),
			"result of addToThisNode(const IOListContainer & tcl) contains only expected paths (paths from original tree or added paths)");
		fsmlib_assert("TC-TreeNode-NNNN",
			resultContainsEachAddedPath(root, iolc1),
			"result of addToThisNode(const IOListContainer & tcl) contains each added path");
		fsmlib_assert("TC-TreeNode-NNNN",
			treeIsObservable(root),
			"result of addToThisNode(const IOListContainer & tcl) contains no redundant prefixes");
	}

	// root has two childs (c1 and c2). c2 is a leaf. c1 has two children (gc1 and gc2). gc1 and gc2 are leaves.
	// IOListContainer contains two paths ({1, 3} and {1, 3, 1}). (first is prefix of second, both have prefix in tree)
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		shared_ptr<TreeNode> c1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> c2 = make_shared<TreeNode>();
		shared_ptr<TreeNode> gc1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> gc2 = make_shared<TreeNode>();
		root->add(make_shared<TreeEdge>(1, c1));
		root->add(make_shared<TreeEdge>(2, c2));
		c1->add(make_shared<TreeEdge>(1, gc1));
		c1->add(make_shared<TreeEdge>(2, gc2));
		shared_ptr<TreeNode> old = root->clone();
		std::vector<int> inputs1 = { 1, 3 };
		std::vector<int> inputs2 = { 1, 3, 1 };
		std::vector<std::vector<int>> ioLst = { inputs1, inputs2 };
		shared_ptr<std::vector<std::vector<int>>> iolLstPtr = make_shared < std::vector<std::vector<int>>>(ioLst);
		shared_ptr<FsmPresentationLayer> presentationLayer = make_shared<FsmPresentationLayer>();
		IOListContainer iolc1(iolLstPtr, presentationLayer);
		root->addToThisNode(iolc1);
		fsmlib_assert("TC-TreeNode-NNNN",
			root->superTreeOf(old),
			"result of addToThisNode(const IOListContainer & tcl) is a super tree of the original TreeNode, so every path from original is still contained in result");
		fsmlib_assert("TC-TreeNode-NNNN",
			resultContainsOnlyExpectedPaths(root, old, iolc1),
			"result of addToThisNode(const IOListContainer & tcl) contains only expected paths (paths from original tree or added paths)");
		fsmlib_assert("TC-TreeNode-NNNN",
			resultContainsEachAddedPath(root, iolc1),
			"result of addToThisNode(const IOListContainer & tcl) contains each added path");
		fsmlib_assert("TC-TreeNode-NNNN",
			treeIsObservable(root),
			"result of addToThisNode(const IOListContainer & tcl) contains no redundant prefixes");
	}

	// root has two childs (c1 and c2). c2 is a leaf. c1 has two children (gc1 and gc2). gc1 and gc2 are leaves.
	// IOListContainer contains three paths ({1, 1} and {1, 2} and {2}). (each path is already contained in tree)
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		shared_ptr<TreeNode> c1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> c2 = make_shared<TreeNode>();
		shared_ptr<TreeNode> gc1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> gc2 = make_shared<TreeNode>();
		root->add(make_shared<TreeEdge>(1, c1));
		root->add(make_shared<TreeEdge>(2, c2));
		c1->add(make_shared<TreeEdge>(1, gc1));
		c1->add(make_shared<TreeEdge>(2, gc2));
		shared_ptr<TreeNode> old = root->clone();
		std::vector<int> inputs1 = { 1, 1 };
		std::vector<int> inputs2 = { 1, 2 };
		std::vector<int> inputs3 = { 2 };
		std::vector<std::vector<int>> ioLst = { inputs1, inputs2, inputs3 };
		shared_ptr<std::vector<std::vector<int>>> iolLstPtr = make_shared < std::vector<std::vector<int>>>(ioLst);
		shared_ptr<FsmPresentationLayer> presentationLayer = make_shared<FsmPresentationLayer>();
		IOListContainer iolc1(iolLstPtr, presentationLayer);
		root->addToThisNode(iolc1);
		fsmlib_assert("TC-TreeNode-NNNN",
			root->superTreeOf(old),
			"result of addToThisNode(const IOListContainer & tcl) is a super tree of the original TreeNode, so every path from original is still contained in result");
		fsmlib_assert("TC-TreeNode-NNNN",
			resultContainsOnlyExpectedPaths(root, old, iolc1),
			"result of addToThisNode(const IOListContainer & tcl) contains only expected paths (paths from original tree or added paths)");
		fsmlib_assert("TC-TreeNode-NNNN",
			resultContainsEachAddedPath(root, iolc1),
			"result of addToThisNode(const IOListContainer & tcl) contains each added path");
		fsmlib_assert("TC-TreeNode-NNNN",
			treeIsObservable(root),
			"result of addToThisNode(const IOListContainer & tcl) contains no redundant prefixes");
	}
}

//===================================== Tree Tests ===================================================

// tests Tree::remove(const std::shared_ptr<Tree> otherTree)
void testTreeRemove() {
	
	// thisTree is a leaf. otherTree is a leaf.
	{
		shared_ptr<TreeNode> thisRoot = make_shared<TreeNode>();
		Tree thisTree(thisRoot, make_shared<FsmPresentationLayer>());

		shared_ptr<TreeNode> otherRoot = make_shared<TreeNode>();
		Tree otherTree(otherRoot, make_shared<FsmPresentationLayer>());

		thisTree.remove(make_shared<Tree>(otherTree));

		fsmlib_assert("TC-Tree-NNNN",
			thisRoot->isDeleted(),
			"remove(const std::shared_ptr<Tree> otherTree): All corresponding nodes (source and target of matching edges) "
			"of thisTree and otherTree are marked as deleted.");
		fsmlib_assert("TC-Tree-NNNN",
			thisRoot->isLeaf(),
			"remove(const std::shared_ptr<Tree> otherTree): Each deleted leaf is removed from children lists");
	}

	// thisTree is a leaf. otherTree has root otherRoot. otherRoot has one child (otherC1). otherC1 is a leaf.
	{
		shared_ptr<TreeNode> thisRoot = make_shared<TreeNode>();
		Tree thisTree(thisRoot, make_shared<FsmPresentationLayer>());

		shared_ptr<TreeNode> otherRoot = make_shared<TreeNode>();
		shared_ptr<TreeNode> otherC1 = make_shared<TreeNode>();
		otherRoot->add(make_shared<TreeEdge>(1, otherC1));
		Tree otherTree(otherRoot, make_shared<FsmPresentationLayer>());

		thisTree.remove(make_shared<Tree>(otherTree));

		fsmlib_assert("TC-Tree-NNNN",
			thisRoot->isDeleted(),
			"remove(const std::shared_ptr<Tree> otherTree): All corresponding nodes (source and target of matching edges) "
			"of thisTree and otherTree are marked as deleted.");
		fsmlib_assert("TC-Tree-NNNN",
			thisRoot->isLeaf(),
			"remove(const std::shared_ptr<Tree> otherTree): Each deleted leaf is removed from children lists");
	}

	// thisTree has root thisRoot. thisRoot has one child (thisC1). thisC1 is a leaf. otherTree is a leaf.
	{
		shared_ptr<TreeNode> thisRoot = make_shared<TreeNode>();
		shared_ptr<TreeNode> thisC1 = make_shared<TreeNode>();
		thisRoot->add(make_shared<TreeEdge>(1, thisC1));
		Tree thisTree(thisRoot, make_shared<FsmPresentationLayer>());

		shared_ptr<TreeNode> otherRoot = make_shared<TreeNode>();
		Tree otherTree(otherRoot, make_shared<FsmPresentationLayer>());

		thisTree.remove(make_shared<Tree>(otherTree));
		
		fsmlib_assert("TC-Tree-NNNN",
			thisRoot->isDeleted()
			&& !(thisC1->isDeleted()),
			"remove(const std::shared_ptr<Tree> otherTree): All corresponding nodes (source and target of matching edges) "
			"of thisTree and otherTree are marked as deleted. Non corresponding nodes aren't deleted.");
		fsmlib_assert("TC-Tree-NNNN",
			!(thisRoot->isLeaf())
			&& thisC1->isLeaf(),
			"remove(const std::shared_ptr<Tree> otherTree): Each deleted leaf is removed from children lists. Each non "
			"deleted leaf isn't removed.");		
	}
	
	// thisTree is super tree of otherTree. thisTree contains two edges emanating from thisRoot. otherTree contains one edge.
	{
		shared_ptr<TreeNode> thisRoot = make_shared<TreeNode>();
		shared_ptr<TreeNode> thisC1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> thisC2 = make_shared<TreeNode>();
		thisRoot->add(make_shared<TreeEdge>(1, thisC1));
		thisRoot->add(make_shared<TreeEdge>(2, thisC2));
		Tree thisTree(thisRoot, make_shared<FsmPresentationLayer>());

		shared_ptr<TreeNode> otherRoot = make_shared<TreeNode>();
		shared_ptr<TreeNode> otherC1 = make_shared<TreeNode>();
		otherRoot->add(make_shared<TreeEdge>(1, otherC1));
		Tree otherTree(otherRoot, make_shared<FsmPresentationLayer>());
		
		thisTree.remove(make_shared<Tree>(otherTree));
		fsmlib_assert("TC-Tree-NNNN",
			thisRoot->isDeleted()
			&& thisC1->isDeleted()
			&& !(thisC2->isDeleted()),
			"remove(const std::shared_ptr<Tree> otherTree): All corresponding nodes (source and target of matching edges) "
			"of thisTree and otherTree are marked as deleted. Non corresponding nodes aren't deleted.");
		fsmlib_assert("TC-Tree-NNNN",
			thisRoot->getChildren()->size() == 1
			&& thisRoot->getChildren()->at(0)->getTarget() == thisC2,
			"remove(const std::shared_ptr<Tree> otherTree): Each deleted leaf is removed from children lists. Each non "
			"deleted leaf isn't removed.");
	}

	// otherTree is super tree of thisTree. thisTree contains two edges emanating from thisRoot. otherTree contains three edges emanating from otherRoot.
	{
		shared_ptr<TreeNode> thisRoot = make_shared<TreeNode>();
		shared_ptr<TreeNode> thisC1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> thisC2 = make_shared<TreeNode>();
		thisRoot->add(make_shared<TreeEdge>(1, thisC1));
		thisRoot->add(make_shared<TreeEdge>(2, thisC2));
		Tree thisTree(thisRoot, make_shared<FsmPresentationLayer>());

		shared_ptr<TreeNode> otherRoot = make_shared<TreeNode>();
		shared_ptr<TreeNode> otherC1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> otherC2 = make_shared<TreeNode>();
		shared_ptr<TreeNode> otherC3 = make_shared<TreeNode>();
		otherRoot->add(make_shared<TreeEdge>(1, otherC1));
		otherRoot->add(make_shared<TreeEdge>(2, otherC2));
		otherRoot->add(make_shared<TreeEdge>(3, otherC3));
		Tree otherTree(otherRoot, make_shared<FsmPresentationLayer>());

		thisTree.remove(make_shared<Tree>(otherTree));

		fsmlib_assert("TC-Tree-NNNN",
			thisRoot->isDeleted()
			&& thisC1->isDeleted()
			&& thisC2->isDeleted(),
			"remove(const std::shared_ptr<Tree> otherTree): All corresponding nodes (source and target of matching edges) "
			"of thisTree and otherTree are marked as deleted. Non corresponding nodes aren't deleted.");
		fsmlib_assert("TC-Tree-NNNN",
			thisRoot->isLeaf(),
			"remove(const std::shared_ptr<Tree> otherTree): Each deleted leaf is removed from children lists. Each non "
			"deleted leaf isn't removed.");
	}	

	// thisTree is super tree of otherTree. height of both trees is 2.
	{
		shared_ptr<TreeNode> thisRoot = make_shared<TreeNode>();
		shared_ptr<TreeNode> thisC1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> thisC2 = make_shared<TreeNode>();
		shared_ptr<TreeNode> thisGC1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> thisGC2 = make_shared<TreeNode>();
		shared_ptr<TreeNode> thisGC3 = make_shared<TreeNode>();
		thisRoot->add(make_shared<TreeEdge>(1, thisC1));
		thisRoot->add(make_shared<TreeEdge>(2, thisC2));
		thisC1->add(make_shared<TreeEdge>(1, thisGC1));
		thisC1->add(make_shared<TreeEdge>(2, thisGC2)); 
		thisC2->add(make_shared<TreeEdge>(1, thisGC3));
		Tree thisTree(thisRoot, make_shared<FsmPresentationLayer>());

		shared_ptr<TreeNode> otherRoot = make_shared<TreeNode>();
		shared_ptr<TreeNode> otherC1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> otherC2 = make_shared<TreeNode>();
		shared_ptr<TreeNode> otherGC1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> otherGC2 = make_shared<TreeNode>();
		otherRoot->add(make_shared<TreeEdge>(1, otherC1));
		otherRoot->add(make_shared<TreeEdge>(2, otherC2));
		otherC1->add(make_shared<TreeEdge>(2, otherGC1));
		otherC2->add(make_shared<TreeEdge>(1, otherGC2));
		Tree otherTree(otherRoot, make_shared<FsmPresentationLayer>());

		thisTree.remove(make_shared<Tree>(otherTree));

		fsmlib_assert("TC-Tree-NNNN",
			thisRoot->isDeleted()
			&& thisC1->isDeleted()
			&& thisC2->isDeleted()
			&& !(thisGC1->isDeleted())
			&& thisGC2->isDeleted()
			&& thisGC3->isDeleted(),
			"remove(const std::shared_ptr<Tree> otherTree): All corresponding nodes (source and target of matching edges) "
			"of thisTree and otherTree are marked as deleted. Non corresponding nodes aren't deleted.");
		fsmlib_assert("TC-Tree-NNNN",
			thisRoot->getChildren()->size() == 1
			&& thisRoot->getChildren()->at(0)->getTarget() == thisC1
			&& thisC1->getChildren()->size() == 1
			&& thisC1->getChildren()->at(0)->getTarget() == thisGC1,
			"remove(const std::shared_ptr<Tree> otherTree): Each deleted leaf is removed from children lists. Each non "
			"deleted leaf isn't removed.");
	}
}

// gets string representation of a Tree::toDot() result and counts the declarations of edges.
int countEdgesInToDotResult(std::string content) {
	int counter = 0;
	int pos = content.find(" -> ", 0);
	while (pos != string::npos) {
		++counter;
		pos = content.find(" -> ", pos + 1);
	}
	return counter;
}

// tests Tree::toDot(ostream & out)
void testTreeToDot() {
	// tree contains only the root (leaf). result contains no edge.
	{
		std::ostringstream stream;
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		Tree tree(root, make_shared<FsmPresentationLayer>());
		tree.toDot(stream);
		std::string content = stream.str();
		fsmlib_assert("TC-Tree-NNNN",
			content.find(" -> ") == string::npos,
			"result of toDot(ostream & out) contains only expected edges");
	}

	
	{
		// root of the tree has one child (c1). c1 is a leaf. result contains 1 edge.
		std::ostringstream stream;
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		shared_ptr<TreeNode> c1 = make_shared<TreeNode>();
		root->add(make_shared<TreeEdge>(1, c1));
		Tree tree(root, make_shared<FsmPresentationLayer>());
		tree.toDot(stream);
		std::string content = stream.str();
		fsmlib_assert("TC-Tree-NNNN",
			content.find("0 -> 1[label = \"1\" ];") != string::npos,
			"result of toDot(ostream & out) contains each expected edge");

		fsmlib_assert("TC-Tree-NNNN",
			countEdgesInToDotResult(content) == 1,
			"result of toDot(ostream & out) contains only expected edges");


		// root has two children (c1 and c2). Both are leaves. result contains two edges.
		shared_ptr<TreeNode> c2 = make_shared<TreeNode>();
		root->add(make_shared<TreeEdge>(2, c2));
		stream.str("");
		stream.clear();
		tree.toDot(stream);
		content = stream.str();

		fsmlib_assert("TC-Tree-NNNN",
			content.find("0 -> 1[label = \"1\" ];") != string::npos
			&& content.find("0 -> 2[label = \"2\" ];") != string::npos,
			"result of toDot(ostream & out) contains each expected edge");

		fsmlib_assert("TC-Tree-NNNN",
			countEdgesInToDotResult(content) == 2,
			"result of toDot(ostream & out) contains only expected edges");

		// root has two children (c1 and c2). c1 has two children (gc1 and gc2). gc1, gc2 and c2 are leaves. result contains 4 edges.
		shared_ptr<TreeNode> gc1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> gc2 = make_shared<TreeNode>();
		c1->add(make_shared<TreeEdge>(1, gc1));
		c1->add(make_shared<TreeEdge>(2, gc2));
		stream.str("");
		stream.clear();
		tree.toDot(stream);
		content = stream.str();

		fsmlib_assert("TC-Tree-NNNN",
			content.find("0 -> 1[label = \"1\" ];") != string::npos
			&& content.find("1 -> 2[label = \"1\" ];") != string::npos
			&& content.find("1 -> 3[label = \"2\" ];") != string::npos
			&& content.find("0 -> 4[label = \"2\" ];") != string::npos,
			"result of toDot(ostream & out) contains each expected edge");

		fsmlib_assert("TC-Tree-NNNN",
			countEdgesInToDotResult(content) == 4,
			"result of toDot(ostream & out) contains only expected edges");
	}

}

// tests Tree::getPrefixRelationTree(const shared_ptr<Tree> & b)
void testTreeGetPrefixRelationTree() {
	// thisTree contains no test case (root without children). otherTree contains two test cases.
	{
		shared_ptr<TreeNode> thisRoot = make_shared<TreeNode>();
		Tree thisTree(thisRoot, make_shared<FsmPresentationLayer>());

		shared_ptr<TreeNode> otherRoot = make_shared<TreeNode>();
		shared_ptr<TreeNode> otherC1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> otherC2 = make_shared<TreeNode>();
		otherRoot->add(make_shared<TreeEdge>(1, otherC1));
		otherRoot->add(make_shared<TreeEdge>(2, otherC2));
		Tree otherTree(otherRoot, make_shared<FsmPresentationLayer>());
		shared_ptr<Tree> otherTreePtr = make_shared<Tree>(otherTree);
		shared_ptr<Tree> result = thisTree.getPrefixRelationTree(otherTreePtr);
		fsmlib_assert("TC-Tree-NNNN",
			result == otherTreePtr,
			"getPrefixRelationTree(const shared_ptr<Tree> & b) returns pointer to b if thisTree contains no test case (root without childs) "
			"and b contains at least one test case.");		
	}

	// thisTree contains two test cases. otherTree contains no test case (root without children).
	{
		shared_ptr<TreeNode> thisRoot = make_shared<TreeNode>();
		shared_ptr<TreeNode> thisC1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> thisC2 = make_shared<TreeNode>();
		thisRoot->add(make_shared<TreeEdge>(1, thisC1));
		thisRoot->add(make_shared<TreeEdge>(2, thisC2));
		shared_ptr<Tree> thisTree = make_shared<Tree>(thisRoot, make_shared<FsmPresentationLayer>());

		shared_ptr<TreeNode> otherRoot = make_shared<TreeNode>();
		shared_ptr<Tree> otherTree = make_shared<Tree>(otherRoot, make_shared<FsmPresentationLayer>());
		shared_ptr<Tree> result = thisTree->getPrefixRelationTree(otherTree);
		fsmlib_assert("TC-Tree-NNNN",
			result == thisTree,
			"getPrefixRelationTree(const shared_ptr<Tree> & b) returns pointer to thisTree if b contains no test case (root without childs) "
			"and thisTree contains at least one test case.");
	}

	// thisTree and otherTree both contain no test case (root without children).
	{
		shared_ptr<TreeNode> thisRoot = make_shared<TreeNode>();
		shared_ptr<Tree> thisTree = make_shared<Tree>(thisRoot, make_shared<FsmPresentationLayer>());

		shared_ptr<TreeNode> otherRoot = make_shared<TreeNode>();
		shared_ptr<Tree> otherTree = make_shared<Tree>(otherRoot, make_shared<FsmPresentationLayer>());
		shared_ptr<Tree> result = thisTree->getPrefixRelationTree(otherTree);
		fsmlib_assert("TC-Tree-NNNN",
			result != thisTree
			&& result != otherTree
			&& result->size() == 1,
			"getPrefixRelationTree(const shared_ptr<Tree> & b) returns pointer to a new empty Tree if b and thisTree are empty.");
	}

	// thisTree contains two test cases. otherTree contains one testcase, which equals one of the test cases of thisTree.
	{
		shared_ptr<TreeNode> thisRoot = make_shared<TreeNode>();
		shared_ptr<TreeNode> thisC1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> thisC2 = make_shared<TreeNode>();
		thisRoot->add(make_shared<TreeEdge>(1, thisC1));
		thisRoot->add(make_shared<TreeEdge>(2, thisC2));
		shared_ptr<Tree> thisTree = make_shared<Tree>(thisRoot, make_shared<FsmPresentationLayer>());

		shared_ptr<TreeNode> otherRoot = make_shared<TreeNode>();
		shared_ptr<TreeNode> otherC1 = make_shared<TreeNode>();
		otherRoot->add(make_shared<TreeEdge>(1, otherC1));
		shared_ptr<Tree> otherTree = make_shared<Tree>(otherRoot, make_shared<FsmPresentationLayer>());
		shared_ptr<Tree> result = thisTree->getPrefixRelationTree(otherTree);
		vector<int> expected = { 1 };
		fsmlib_assert("TC-Tree-NNNN",
			result->getTestCases().getIOLists()->size() == 1
			&& result->getTestCases().getIOLists()->at(0) == expected,
			"result of getPrefixRelationTree(const shared_ptr<Tree> & b) contains each expected test case and no unexpected test case.");
	}

	// thisTree contains two test cases. otherTree contains only one testcase. Trees don't share any prefixes.
	{
		shared_ptr<TreeNode> thisRoot = make_shared<TreeNode>();
		shared_ptr<TreeNode> thisC1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> thisC2 = make_shared<TreeNode>();
		thisRoot->add(make_shared<TreeEdge>(1, thisC1));
		thisRoot->add(make_shared<TreeEdge>(2, thisC2));
		shared_ptr<Tree> thisTree = make_shared<Tree>(thisRoot, make_shared<FsmPresentationLayer>());

		shared_ptr<TreeNode> otherRoot = make_shared<TreeNode>();
		shared_ptr<TreeNode> otherC1 = make_shared<TreeNode>();
		otherRoot->add(make_shared<TreeEdge>(3, otherC1));
		shared_ptr<Tree> otherTree = make_shared<Tree>(otherRoot, make_shared<FsmPresentationLayer>());
		shared_ptr<Tree> result = thisTree->getPrefixRelationTree(otherTree);
		fsmlib_assert("TC-Tree-NNNN",
			result->size() == 1,
			"result of getPrefixRelationTree(const shared_ptr<Tree> & b) contains each expected test case and no unexpected test case.");
	}

	// thisTree contains two test cases (thisTC1 and thisTC2). otherTree contains three testcase (otherTC1, otherTC2 and otherTC3). 
	// otherTC1 is a prefix of thisTC1. thisTC2 is a prefix of otherTC2 and otherTC3.
	// (Each test case is either a prefix of a test case contained in the other tree or has a prefix in the other tree)
	{
		shared_ptr<TreeNode> thisRoot = make_shared<TreeNode>();
		shared_ptr<TreeNode> thisC1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> thisC2 = make_shared<TreeNode>();
		shared_ptr<TreeNode> thisGC1 = make_shared<TreeNode>();
		thisRoot->add(make_shared<TreeEdge>(1, thisC1));
		thisRoot->add(make_shared<TreeEdge>(2, thisC2));
		thisC1->add(make_shared<TreeEdge>(1, thisGC1));
		shared_ptr<Tree> thisTree = make_shared<Tree>(thisRoot, make_shared<FsmPresentationLayer>());

		shared_ptr<TreeNode> otherRoot = make_shared<TreeNode>();
		shared_ptr<TreeNode> otherC1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> otherC2 = make_shared<TreeNode>();
		shared_ptr<TreeNode> otherGC1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> otherGC2 = make_shared<TreeNode>();
		otherRoot->add(make_shared<TreeEdge>(1, otherC1));
		otherRoot->add(make_shared<TreeEdge>(2, otherC2));
		otherC2->add(make_shared<TreeEdge>(2, otherGC1));
		otherC2->add(make_shared<TreeEdge>(1, otherGC2));
		shared_ptr<Tree> otherTree = make_shared<Tree>(otherRoot, make_shared<FsmPresentationLayer>());
		shared_ptr<Tree> result = thisTree->getPrefixRelationTree(otherTree);

		vector<int> thisTC1 = { 1,1 };
		vector<int> otherTC2 = { 2,2 };
		vector<int> otherTC3 = { 2,1 };

		shared_ptr<vector<vector<int>>> testCases = result->getTestCases().getIOLists();

		fsmlib_assert("TC-Tree-NNNN",
			testCases->size() == 3
			&& find(testCases->cbegin(), testCases->cend(), thisTC1) != testCases->cend()
			&& find(testCases->cbegin(), testCases->cend(), otherTC2) != testCases->cend()
			&& find(testCases->cbegin(), testCases->cend(), otherTC3) != testCases->cend(),
			"result of getPrefixRelationTree(const shared_ptr<Tree> & b) contains each expected test case and no unexpected test case.");
	}

	// thisTree contains two test cases (thisTC1 and thisTC2). otherTree contains three testcase (otherTC1, otherTC2 and otherTC3). 
	// otherTC1 is a prefix of thisTC1. thisTC2, otherTC2 and otherTC3 aren't prefixes of each other (but share prefixes).
	{
		shared_ptr<TreeNode> thisRoot = make_shared<TreeNode>();
		shared_ptr<TreeNode> thisC1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> thisC2 = make_shared<TreeNode>();
		shared_ptr<TreeNode> thisGC1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> thisGC2 = make_shared<TreeNode>();
		thisRoot->add(make_shared<TreeEdge>(1, thisC1));
		thisRoot->add(make_shared<TreeEdge>(2, thisC2));
		thisC1->add(make_shared<TreeEdge>(1, thisGC1));
		thisC2->add(make_shared<TreeEdge>(3, thisGC2));
		shared_ptr<Tree> thisTree = make_shared<Tree>(thisRoot, make_shared<FsmPresentationLayer>());

		shared_ptr<TreeNode> otherRoot = make_shared<TreeNode>();
		shared_ptr<TreeNode> otherC1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> otherC2 = make_shared<TreeNode>();
		shared_ptr<TreeNode> otherGC1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> otherGC2 = make_shared<TreeNode>();
		otherRoot->add(make_shared<TreeEdge>(1, otherC1));
		otherRoot->add(make_shared<TreeEdge>(2, otherC2));
		otherC2->add(make_shared<TreeEdge>(2, otherGC1));
		otherC2->add(make_shared<TreeEdge>(1, otherGC2));
		shared_ptr<Tree> otherTree = make_shared<Tree>(otherRoot, make_shared<FsmPresentationLayer>());
		shared_ptr<Tree> result = thisTree->getPrefixRelationTree(otherTree);

		vector<int> thisTC1 = { 1,1 };

		shared_ptr<vector<vector<int>>> testCases = result->getTestCases().getIOLists();

		fsmlib_assert("TC-Tree-NNNN",
			testCases->size() == 1
			&& find(testCases->cbegin(), testCases->cend(), thisTC1) != testCases->cend(),
			"result of getPrefixRelationTree(const shared_ptr<Tree> & b) contains each expected test case and no unexpected test case.");
	}
}

// tests Tree::tentativeAddToRoot(SegmentedTrace& alpha)
void testTreeTentativeAddToRoot() {
	// case 1: alpha is already contained in Tree.

	// root of Tree is a leaf. alpha = <>
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		shared_ptr<Tree> tree = make_shared<Tree>(root, make_shared<FsmPresentationLayer>());

		vector<int> seg1vec = { };
		shared_ptr<TraceSegment> seg1 = make_shared<TraceSegment>(make_shared<vector<int>>(seg1vec));
		std::deque< std::shared_ptr<TraceSegment> > segments = {seg1};
		SegmentedTrace alpha(segments);
	
		fsmlib_assert("TC-Tree-NNNN",
			tree->tentativeAddToRoot(alpha) == 0,
			"Tree::tentativeAddToRoot(SegmentedTrace& alpha) return 0 if alpha is already contained in Tree.");
	}

	// Tree contains one path (2 edges). alpha matches this path (with one, two or three segments)
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		shared_ptr<TreeNode> c1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> gc1 = make_shared<TreeNode>();
		root->add(make_shared<TreeEdge>(1, c1));
		c1->add(make_shared<TreeEdge>(2, gc1));
		shared_ptr<Tree> tree = make_shared<Tree>(root, make_shared<FsmPresentationLayer>());

		// alpha = 2 segments
		vector<int> seg1vec = { 1 };				
		shared_ptr<TraceSegment> seg1 = make_shared<TraceSegment>(make_shared<vector<int>>(seg1vec));
		vector<int> seg2vec = { 2 };
		shared_ptr<TraceSegment> seg2 = make_shared<TraceSegment>(make_shared<vector<int>>(seg2vec));
		std::deque< std::shared_ptr<TraceSegment> > segments = { seg1, seg2 };
		SegmentedTrace alpha(segments);

		fsmlib_assert("TC-Tree-NNNN",
			tree->tentativeAddToRoot(alpha) == 0,
			"Tree::tentativeAddToRoot(SegmentedTrace& alpha) return 0 if alpha is already contained in Tree.");

		// alpha = 1 segment
		seg1vec = { 1, 2 };
		seg1 = make_shared<TraceSegment>(make_shared<vector<int>>(seg1vec));
		segments = { seg1 };
		alpha = segments;

		fsmlib_assert("TC-Tree-NNNN",
			tree->tentativeAddToRoot(alpha) == 0,
			"Tree::tentativeAddToRoot(SegmentedTrace& alpha) return 0 if alpha is already contained in Tree.");

		// alpha = 3 segments
		seg1vec = { 1 };
		seg1 = make_shared<TraceSegment>(make_shared<vector<int>>(seg1vec));
		seg2vec = { 2 };
		seg2 = make_shared<TraceSegment>(make_shared<vector<int>>(seg2vec));
		vector<int> seg3vec = {  };
		shared_ptr<TraceSegment> seg3 = make_shared<TraceSegment>(make_shared<vector<int>>(seg3vec));
		segments = { seg1, seg2, seg3 };
		alpha = segments;

		fsmlib_assert("TC-Tree-NNNN",
			tree->tentativeAddToRoot(alpha) == 0,
			"Tree::tentativeAddToRoot(SegmentedTrace& alpha) return 0 if alpha is already contained in Tree.");
	}

	// Bigger Tree with longer paths. alpha is already contained.
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		shared_ptr<TreeNode> c1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> c2 = make_shared<TreeNode>();
		shared_ptr<TreeNode> gc1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> gc2 = make_shared<TreeNode>();
		shared_ptr<TreeNode> ggc1 = make_shared<TreeNode>();
		root->add(make_shared<TreeEdge>(1, c1));
		root->add(make_shared<TreeEdge>(2, c2));
		c1->add(make_shared<TreeEdge>(2, gc1));
		c2->add(make_shared<TreeEdge>(3, gc2));
		gc1->add(make_shared<TreeEdge>(3, ggc1));
		shared_ptr<Tree> tree = make_shared<Tree>(root, make_shared<FsmPresentationLayer>());

		// alpha = 2 segments. alpha is prefix of a path in Tree
		vector<int> seg1vec = { 1 };
		shared_ptr<TraceSegment> seg1 = make_shared<TraceSegment>(make_shared<vector<int>>(seg1vec));
		vector<int> seg2vec = { 2 };
		shared_ptr<TraceSegment> seg2 = make_shared<TraceSegment>(make_shared<vector<int>>(seg2vec));
		std::deque< std::shared_ptr<TraceSegment> > segments = { seg1, seg2 };
		SegmentedTrace alpha(segments);

		fsmlib_assert("TC-Tree-NNNN",
			tree->tentativeAddToRoot(alpha) == 0,
			"Tree::tentativeAddToRoot(SegmentedTrace& alpha) return 0 if alpha is already contained in Tree.");

		/// alpha = 2 segments. alpha is prefix of a path in Tree
		seg1vec = { 1, 2 };
		seg1 = make_shared<TraceSegment>(make_shared<vector<int>>(seg1vec));
		seg2vec = { 3 };
		seg2 = make_shared<TraceSegment>(make_shared<vector<int>>(seg2vec));
		segments = { seg1, seg2 };
		alpha = segments;

		fsmlib_assert("TC-Tree-NNNN",
			tree->tentativeAddToRoot(alpha) == 0,
			"Tree::tentativeAddToRoot(SegmentedTrace& alpha) return 0 if alpha is already contained in Tree.");
	}

	// case 2: prefix of path alpha reaches leaf of tree (no new branch needed) -> returns 1

	// root of the Tree is a leaf. alpha is not empty.
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		shared_ptr<Tree> tree = make_shared<Tree>(root, make_shared<FsmPresentationLayer>());

		// alpha = 2 segments.
		vector<int> seg1vec = { 1 };
		shared_ptr<TraceSegment> seg1 = make_shared<TraceSegment>(make_shared<vector<int>>(seg1vec));
		vector<int> seg2vec = { 2 };
		shared_ptr<TraceSegment> seg2 = make_shared<TraceSegment>(make_shared<vector<int>>(seg2vec));
		std::deque< std::shared_ptr<TraceSegment> > segments = { seg1, seg2 };
		SegmentedTrace alpha(segments);

		fsmlib_assert("TC-Tree-NNNN",
			tree->tentativeAddToRoot(alpha) == 1,
			"Tree::tentativeAddToRoot(SegmentedTrace& alpha) return 1 if prefix of alpha reaches leaf of Tree.");
	}

	// root of the Tree has two childs (c1 and c2). c1 and c2 are leaves. Prefix of alpha reaches c1. 
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		shared_ptr<TreeNode> c1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> c2 = make_shared<TreeNode>();
		root->add(make_shared<TreeEdge>(1, c1));
		root->add(make_shared<TreeEdge>(2, c2));
		shared_ptr<Tree> tree = make_shared<Tree>(root, make_shared<FsmPresentationLayer>());

		// alpha = 1 segment (<1,2>).
		vector<int> seg1vec = { 1, 2 };
		shared_ptr<TraceSegment> seg1 = make_shared<TraceSegment>(make_shared<vector<int>>(seg1vec));
		std::deque< std::shared_ptr<TraceSegment> > segments = { seg1 };
		SegmentedTrace alpha(segments);

		fsmlib_assert("TC-Tree-NNNN",
			tree->tentativeAddToRoot(alpha) == 1,
			"Tree::tentativeAddToRoot(SegmentedTrace& alpha) return 1 if prefix of alpha reaches leaf of Tree.");

		// alpha = 2 segments (<1>.<2>).
		seg1vec = { 1 };
		seg1 = make_shared<TraceSegment>(make_shared<vector<int>>(seg1vec));
		vector<int> seg2vec = { 2 };
		shared_ptr<TraceSegment> seg2 = make_shared<TraceSegment>(make_shared<vector<int>>(seg2vec));
		segments = { seg1, seg2 };
		alpha = segments;

		fsmlib_assert("TC-Tree-NNNN",
			tree->tentativeAddToRoot(alpha) == 1,
			"Tree::tentativeAddToRoot(SegmentedTrace& alpha) return 1 if prefix of alpha reaches leaf of Tree.");
	}

	// root of the Tree has two childs (c1 and c2). c1 has one child (gc1).  gc1 is a leaf. c2 has a child (gc2). gc2 is a leaf.
	// Prefix of alpha reaches gc1. 
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		shared_ptr<TreeNode> c1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> c2 = make_shared<TreeNode>();
		shared_ptr<TreeNode> gc1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> gc2 = make_shared<TreeNode>();
		root->add(make_shared<TreeEdge>(1, c1));
		c1->add(make_shared<TreeEdge>(2, gc1));
		root->add(make_shared<TreeEdge>(2, c2));
		c2->add(make_shared<TreeEdge>(3, gc2));
		shared_ptr<Tree> tree = make_shared<Tree>(root, make_shared<FsmPresentationLayer>());

		// alpha = 2 segments (<1,2>.<3>).
		vector<int> seg1vec = { 1, 2 };
		shared_ptr<TraceSegment> seg1 = make_shared<TraceSegment>(make_shared<vector<int>>(seg1vec));
		vector<int> seg2vec = { 3 };
		shared_ptr<TraceSegment> seg2 = make_shared<TraceSegment>(make_shared<vector<int>>(seg2vec));
		std::deque< std::shared_ptr<TraceSegment> > segments = { seg1, seg2 };
		SegmentedTrace alpha(segments);

		fsmlib_assert("TC-Tree-NNNN",
			tree->tentativeAddToRoot(alpha) == 1,
			"Tree::tentativeAddToRoot(SegmentedTrace& alpha) return 1 if prefix of alpha reaches leaf of Tree.");

		// alpha = 2 segments (<1>.<2,3>).
		seg1vec = { 1 };
		seg1 = make_shared<TraceSegment>(make_shared<vector<int>>(seg1vec));
		seg2vec = { 2, 3 };
		seg2 = make_shared<TraceSegment>(make_shared<vector<int>>(seg2vec));
		segments = { seg1, seg2 };
		alpha = segments;

		fsmlib_assert("TC-Tree-NNNN",
			tree->tentativeAddToRoot(alpha) == 1,
			"Tree::tentativeAddToRoot(SegmentedTrace& alpha) return 1 if prefix of alpha reaches leaf of Tree.");
	}

	// case 3: path not fully contained and no prefix of path reaches leaf of tree (new branch needed) -> returns 2

	// root of the Tree has one childs (c1). c1 is a leaf. First element of alpha has no corresponding edge in the Tree.
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		shared_ptr<TreeNode> c1 = make_shared<TreeNode>();
		root->add(make_shared<TreeEdge>(1, c1));
		shared_ptr<Tree> tree = make_shared<Tree>(root, make_shared<FsmPresentationLayer>());

		// alpha = 1 segment (<2>).
		vector<int> seg1vec = { 2 };
		shared_ptr<TraceSegment> seg1 = make_shared<TraceSegment>(make_shared<vector<int>>(seg1vec));
		std::deque< std::shared_ptr<TraceSegment> > segments = { seg1 };
		SegmentedTrace alpha(segments);

		fsmlib_assert("TC-Tree-NNNN",
			tree->tentativeAddToRoot(alpha) == 2,
			"Tree::tentativeAddToRoot(SegmentedTrace& alpha) return 2 if prefix of alpha reaches leaf of Tree.");

		// alpha = 2 segments (<2>.<1>)
		vector<int> seg2vec = { 1 };
		shared_ptr<TraceSegment> seg2 = make_shared<TraceSegment>(make_shared<vector<int>>(seg2vec));
		segments = { seg1, seg2 };
		alpha = segments;

		fsmlib_assert("TC-Tree-NNNN",
			tree->tentativeAddToRoot(alpha) == 2,
			"Tree::tentativeAddToRoot(SegmentedTrace& alpha) return 2 if prefix of alpha reaches leaf of Tree.");

	}

	// root of the Tree has one child (c1). c1 has one child (gc1). gc1 is a leaf.
	// Prefix of alpha matches a prefix of a path contained in the Tree, but alpha contains additional elements (new Branch needed).
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		shared_ptr<TreeNode> c1 = make_shared<TreeNode>();
		shared_ptr<TreeNode> gc1 = make_shared<TreeNode>();
		root->add(make_shared<TreeEdge>(1, c1));
		c1->add(make_shared<TreeEdge>(2, gc1));
		shared_ptr<Tree> tree = make_shared<Tree>(root, make_shared<FsmPresentationLayer>());

		// alpha = 1 segment (<1, 1>).
		vector<int> seg1vec = { 1, 1 };
		shared_ptr<TraceSegment> seg1 = make_shared<TraceSegment>(make_shared<vector<int>>(seg1vec));
		std::deque< std::shared_ptr<TraceSegment> > segments = { seg1 };
		SegmentedTrace alpha(segments);

		fsmlib_assert("TC-Tree-NNNN",
			tree->tentativeAddToRoot(alpha) == 2,
			"Tree::tentativeAddToRoot(SegmentedTrace& alpha) return 2 if prefix of alpha reaches leaf of Tree.");

		// alpha = 2 segments (<1>.<1>)
		seg1vec = { 1 };
		seg1 = make_shared<TraceSegment>(make_shared<vector<int>>(seg1vec));
		vector<int> seg2vec = { 1 };
		shared_ptr<TraceSegment> seg2 = make_shared<TraceSegment>(make_shared<vector<int>>(seg2vec));
		segments = { seg1, seg2 };
		alpha = segments;

		fsmlib_assert("TC-Tree-NNNN",
			tree->tentativeAddToRoot(alpha) == 2,
			"Tree::tentativeAddToRoot(SegmentedTrace& alpha) return 2 if prefix of alpha reaches leaf of Tree.");

	}
}

//===================================== FsmPresentationLayer Tests ===================================================

// tests FsmPresentationLayer(const std::string & inputs, const std::string & outputs, const std::string & states);
void testFsmPresentationLayerFileConstructor() {
	// Correct file names. No file is empty.
	shared_ptr<FsmPresentationLayer> pl =
		make_shared<FsmPresentationLayer>("../../../resources/garageIn.txt",
			"../../../resources/garageOut.txt",
			"../../../resources/garageState.txt");

	vector<string> inputs{ "e1", "e2", "e3", "e4" };
	vector<string> outputs{ "a0", "a1", "a2", "a3", "a4"};
	vector<string> states{ "Door Up", "Door Down", "Door stopped going down", "Door stopped going up", "Door closing", "Door opening" };
	fsmlib_assert("TC-FsmPresentationLayer-NNNN",
		pl->getIn2String() == inputs
		&& pl->getOut2String() == outputs
		&& pl->getState2String() == states,
		"FsmPresentationLayer(const std::string & inputs, const std::string & outputs, const std::string & states) correctly initializes "
		"contents of in2String, out2String and state2String");

	// input file doesn't exist
	pl =
		make_shared<FsmPresentationLayer>("nonExistingFileName.txt",
			"../../../resources/garageOut.txt",
			"../../../resources/garageState.txt");
	inputs = {  };
	outputs = { "a0", "a1", "a2", "a3", "a4" };
	states = { "Door Up", "Door Down", "Door stopped going down", "Door stopped going up", "Door closing", "Door opening" };
	fsmlib_assert("TC-FsmPresentationLayer-NNNN",
		pl->getIn2String() == inputs
		&& pl->getOut2String() == outputs
		&& pl->getState2String() == states,
		"FsmPresentationLayer(const std::string & inputs, const std::string & outputs, const std::string & states) correctly initializes "
		"contents of in2String, out2String and state2String");

	// Using the same file name for each parameter.
	pl =
		make_shared<FsmPresentationLayer>("../../../resources/garageIn.txt",
			"../../../resources/garageIn.txt",
			"../../../resources/garageIn.txt");
	inputs = { "e1", "e2", "e3", "e4" };
	outputs = { "e1", "e2", "e3", "e4" };
	states = { "e1", "e2", "e3", "e4" };
	fsmlib_assert("TC-FsmPresentationLayer-NNNN",
		pl->getIn2String() == inputs
		&& pl->getOut2String() == outputs
		&& pl->getState2String() == states,
		"FsmPresentationLayer(const std::string & inputs, const std::string & outputs, const std::string & states) correctly initializes "
		"contents of in2String, out2String and state2String");

	// One file is completely empty
	pl =
		make_shared<FsmPresentationLayer>("../../../resources/emptyIn.txt",
			"../../../resources/garageOut.txt",
			"../../../resources/garageState.txt");
	inputs = {  };
	outputs = { "a0", "a1", "a2", "a3", "a4" };
	states = { "Door Up", "Door Down", "Door stopped going down", "Door stopped going up", "Door closing", "Door opening" };
	fsmlib_assert("TC-FsmPresentationLayer-NNNN",
		pl->getIn2String() == inputs
		&& pl->getOut2String() == outputs
		&& pl->getState2String() == states,
		"FsmPresentationLayer(const std::string & inputs, const std::string & outputs, const std::string & states) correctly initializes "
		"contents of in2String, out2String and state2String");
}

// tests FsmPresentationLayer::dumpIn(std::ostream & out)
void testFsmPresentationLayerDumpIn() {
	// in2String is empty
	shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
	ostringstream stream;
	pl->dumpIn(stream);
	string s = stream.str();
	fsmlib_assert("TC-FsmPresentationLayer-NNNN",
		s.empty(),
		"FsmPresentationLayer::dumpIn(std::ostream & out) writes each element of in2String to out.");

	stream.str("");
	stream.clear();

	// in2String contains only one element
	vector<string> in2String{ "e1" };
	vector<string> out2String{ "o1", "o2" };
	vector<string> state2String{ "s1" };
	pl = make_shared<FsmPresentationLayer>(in2String, out2String, state2String);
	pl->dumpIn(stream);
	s = stream.str();
	fsmlib_assert("TC-FsmPresentationLayer-NNNN",
		s == "e1",
		"FsmPresentationLayer::dumpIn(std::ostream & out) writes each element of in2String to out.");

	stream.str("");
	stream.clear();

	// in2String contains two elements
	in2String = { "e1", "e2" };
	pl = make_shared<FsmPresentationLayer>(in2String, out2String, state2String);
	pl->dumpIn(stream);
	s = stream.str();
	fsmlib_assert("TC-FsmPresentationLayer-NNNN",
		s == "e1\ne2",
		"FsmPresentationLayer::dumpIn(std::ostream & out) writes each element of in2String to out.");
}

// tests FsmPresentationLayer::compare(std::shared_ptr<FsmPresentationLayer> otherPresentationLayer)
// Positive Case
void testFsmPresentationLayerComparePositive() {
	// in2String1 and in2String2 are empty. out2String1 and out2String2 are empty. state2String1 == state2String
	vector<string> in2String1{};
	vector<string> out2String1{};
	vector<string> state2String1{};
	shared_ptr<FsmPresentationLayer> pl1 = make_shared<FsmPresentationLayer>(in2String1, out2String1, state2String1);

	vector<string> in2String2{};
	vector<string> out2String2{};
	vector<string> state2String2{};
	shared_ptr<FsmPresentationLayer> pl2 = make_shared<FsmPresentationLayer>(in2String2, out2String2, state2String2);

	fsmlib_assert("TC-FsmPresentationLayer-NNNN",
		pl1->compare(pl2)
		&& pl2->compare(pl1),
		"FsmPresentationLayer::compare(std::shared_ptr<FsmPresentationLayer>) returns "
		"true if both in2String and out2String lists are equal.");

	// in2String and out2String lists are equals but not empty. state2String lists differ in size.
	in2String1 = { "e1" };
	in2String2 = { "e1" };
	out2String1 = { "o1", "o2" };
	out2String2 = { "o1", "o2" };
	state2String1 = {};
	state2String2 = { "s1" };

	pl1 = make_shared<FsmPresentationLayer>(in2String1, out2String1, state2String1);
	pl2 = make_shared<FsmPresentationLayer>(in2String2, out2String2, state2String2);

	fsmlib_assert("TC-FsmPresentationLayer-NNNN",
		pl1->compare(pl2)
		&& pl2->compare(pl1),
		"FsmPresentationLayer::compare(std::shared_ptr<FsmPresentationLayer>) returns "
		"true if both in2String and out2String lists are equal.");

}

// tests FsmPresentationLayer::compare(std::shared_ptr<FsmPresentationLayer> otherPresentationLayer)
// Positive Case
void testFsmPresentationLayerCompareNegative() {
	// in2String lists differ in size. out2String lists are equal.
	vector<string> in2String1{};
	vector<string> out2String1{"o1"};
	vector<string> state2String1{};
	shared_ptr<FsmPresentationLayer> pl1 = make_shared<FsmPresentationLayer>(in2String1, out2String1, state2String1);

	vector<string> in2String2{ "e1" };
	vector<string> out2String2{"o1"};
	vector<string> state2String2{};
	shared_ptr<FsmPresentationLayer> pl2 = make_shared<FsmPresentationLayer>(in2String2, out2String2, state2String2);

	fsmlib_assert("TC-FsmPresentationLayer-NNNN",
		!pl1->compare(pl2)
		&& !pl2->compare(pl1),
		"FsmPresentationLayer::compare(std::shared_ptr<FsmPresentationLayer>) returns "
		"false if both in2String lists differ in size");

	// in2String lists are equals. out2String lists differ in size.
	in2String1 = { "e1" };
	in2String2 = { "e1" };
	out2String1 = { "o1" };
	out2String2 = {  };
	state2String1 = {};
	state2String2 = {};

	pl1 = make_shared<FsmPresentationLayer>(in2String1, out2String1, state2String1);
	pl2 = make_shared<FsmPresentationLayer>(in2String2, out2String2, state2String2);

	fsmlib_assert("TC-FsmPresentationLayer-NNNN",
		!pl1->compare(pl2)
		&& !pl2->compare(pl1),
		"FsmPresentationLayer::compare(std::shared_ptr<FsmPresentationLayer>) returns "
		"false if both out2String lists differ in size.");

	// in2String lists are equal. out2String lists have the same size but contain at least one different element.
	in2String1 = { "e1", "e2" };
	in2String2 = { "e1", "e2" };
	out2String1 = { "o1", "o2" };
	out2String2 = { "o1", "o3" };
	state2String1 = {};
	state2String2 = {};

	pl1 = make_shared<FsmPresentationLayer>(in2String1, out2String1, state2String1);
	pl2 = make_shared<FsmPresentationLayer>(in2String2, out2String2, state2String2);

	fsmlib_assert("TC-FsmPresentationLayer-NNNN",
		!pl1->compare(pl2)
		&& !pl2->compare(pl1),
		"FsmPresentationLayer::compare(std::shared_ptr<FsmPresentationLayer>) returns "
		"false if both out2String lists contain different elements.");

	// out2String lists are equal. in2String lists have the same size but contain at least one different element.
	in2String1 = { "e1", "e2" };
	in2String2 = { "e2", "e1" };
	out2String1 = { "o1", "o2" };
	out2String2 = { "o1", "o2" };
	state2String1 = {};
	state2String2 = {};

	pl1 = make_shared<FsmPresentationLayer>(in2String1, out2String1, state2String1);
	pl2 = make_shared<FsmPresentationLayer>(in2String2, out2String2, state2String2);

	fsmlib_assert("TC-FsmPresentationLayer-NNNN",
		!pl1->compare(pl2)
		&& !pl2->compare(pl1),
		"FsmPresentationLayer::compare(std::shared_ptr<FsmPresentationLayer>) returns "
		"false if both out2String lists contain different elements.");
}

//===================================== Trace Tests ===================================================

// tests operator==(Trace const & trace1, Trace const & trace2)
// Positive case.
void testTraceEquals1Positive() {
	// tr1 and tr2 are empty.
	vector<int> v1 = {};
	Trace tr1{ v1, make_shared<FsmPresentationLayer>() };

	vector<int> v2 = {};
	Trace tr2{ v2, make_shared<FsmPresentationLayer>() };

	fsmlib_assert("TC-Trace-NNNN",
		tr1 == tr2,
		"tr1 == tr2 if the underlying vectors are equal.");

	// tr1 and tr2 both contain one element.
	v1 = {1};
	v2 = {1};
	tr1 = { v1, make_shared<FsmPresentationLayer>() };
	tr2 = { v2, make_shared<FsmPresentationLayer>() };

	fsmlib_assert("TC-Trace-NNNN",
		tr1 == tr2,
		"tr1 == tr2 if the underlying vectors are equal.");

	// tr1 and tr2 both contain two elements.
	v1 = { 1, 2 };
	v2 = { 1, 2 };
	tr1 = { v1, make_shared<FsmPresentationLayer>() };
	tr2 = { v2, make_shared<FsmPresentationLayer>() };

	fsmlib_assert("TC-Trace-NNNN",
		tr1 == tr2,
		"tr1 == tr2 if the underlying vectors are equal.");
}

// tests operator==(Trace const & trace1, Trace const & trace2)
// Negative case.
void testTraceEquals1Negative() {
	// tr1 is empty. tr2 isn't empty.
	vector<int> v1 = {};
	Trace tr1{ v1, make_shared<FsmPresentationLayer>() };

	vector<int> v2 = { 1 };
	Trace tr2{ v2, make_shared<FsmPresentationLayer>() };

	fsmlib_assert("TC-Trace-NNNN",
		not (tr1 == tr2),
		"tr1 == tr2 is false if the underlying vectors are unequal.");

	// tr2 is empty. tr1 isn't empty.
	v1 = { 1 };
	v2 = {  };
	tr1 = { v1, make_shared<FsmPresentationLayer>() };
	tr2 = { v2, make_shared<FsmPresentationLayer>() };

	fsmlib_assert("TC-Trace-NNNN",
		not (tr1 == tr2),
		"tr1 == tr2 is false if the underlying vectors are unequal.");

	// tr1 and tr2 have the same size but contain different elements.
	v1 = { 1 };
	v2 = { 2 };
	tr1 = { v1, make_shared<FsmPresentationLayer>() };
	tr2 = { v2, make_shared<FsmPresentationLayer>() };

	fsmlib_assert("TC-Trace-NNNN",
		not (tr1 == tr2),
		"tr1 == tr2 is false if the underlying vectors are unequal.");

	// tr1 and tr2 have the same size but contain different elements.
	v1 = { 1, 2 };
	v2 = { 1, 3 };
	tr1 = { v1, make_shared<FsmPresentationLayer>() };
	tr2 = { v2, make_shared<FsmPresentationLayer>() };

	fsmlib_assert("TC-Trace-NNNN",
		not (tr1 == tr2),
		"tr1 == tr2 is false if the underlying vectors are unequal.");
}

// tests operator==(Trace const & trace1, std::vector<int> const & trace2)
// Positive case.
void testTraceEquals2Positive() {
	// tr1 and tr2 are empty.
	vector<int> v1 = {};
	Trace tr1{ v1, make_shared<FsmPresentationLayer>() };

	vector<int> tr2 = {};

	fsmlib_assert("TC-Trace-NNNN",
		tr1 == tr2,
		"tr1 == tr2 if the underlying vectors are equal.");

	// tr1 and tr2 both contain one element.
	v1 = { 1 };
	tr2 = { 1 };
	tr1 = { v1, make_shared<FsmPresentationLayer>() };

	fsmlib_assert("TC-Trace-NNNN",
		tr1 == tr2,
		"tr1 == tr2 if the underlying vectors are equal.");

	// tr1 and tr2 both contain two elements.
	v1 = { 1, 2 };
	tr2 = { 1, 2 };
	tr1 = { v1, make_shared<FsmPresentationLayer>() };

	fsmlib_assert("TC-Trace-NNNN",
		tr1 == tr2,
		"tr1 == tr2 if the underlying vectors are equal.");
}

// tests operator==(Trace const & trace1, std::vector<int> const & trace2)
// Negative case.
void testTraceEquals2Negative() {
	// tr1 is empty. tr2 isn't empty.
	vector<int> v1 = {};
	Trace tr1{ v1, make_shared<FsmPresentationLayer>() };

	vector<int> tr2 = { 1 };

	fsmlib_assert("TC-Trace-NNNN",
		not (tr1 == tr2),
		"tr1 == tr2 is false if the underlying vectors are unequal.");

	// tr2 is empty. tr1 isn't empty.
	v1 = { 1 };
	tr2 = {};
	tr1 = { v1, make_shared<FsmPresentationLayer>() };

	fsmlib_assert("TC-Trace-NNNN",
		not (tr1 == tr2),
		"tr1 == tr2 is false if the underlying vectors are unequal.");

	// tr1 and tr2 have the same size but contain different elements.
	v1 = { 1 };
	tr2 = { 2 };
	tr1 = { v1, make_shared<FsmPresentationLayer>() };

	fsmlib_assert("TC-Trace-NNNN",
		not (tr1 == tr2),
		"tr1 == tr2 is false if the underlying vectors are unequal.");

	// tr1 and tr2 have the same size but contain different elements.
	v1 = { 1, 2 };
	tr2 = { 1, 3 };
	tr1 = { v1, make_shared<FsmPresentationLayer>() };

	fsmlib_assert("TC-Trace-NNNN",
		not (tr1 == tr2),
		"tr1 == tr2 is false if the underlying vectors are unequal.");
}

// tests operator<<(std::ostream & out, const Trace & trace)
void testTraceOutputOperator() {
	// trace is empty.
	vector<int> v1 = {};
	Trace tr1{ v1, make_shared<FsmPresentationLayer>() };
	ostringstream out;
	out << tr1;
	string result = out.str();

	fsmlib_assert("TC-Trace-NNNN",
		result == "",
		"operator<<(std::ostream & out, const Trace & trace) writes every element of trace to out in the right order.");

	out.str("");
	out.clear();

	// trace contains one element.
	v1 = { 1 };
	tr1 = { v1,  make_shared<FsmPresentationLayer>() };
	out << tr1;
	result = out.str();

	fsmlib_assert("TC-Trace-NNNN",
		result == "1",
		"operator<<(std::ostream & out, const Trace & trace) writes every element of trace to out in the right order.");

	out.str("");
	out.clear();

	// trace contains two elements.
	v1 = { 1,2 };
	tr1 = { v1,  make_shared<FsmPresentationLayer>() };
	out << tr1;
	result = out.str();

	fsmlib_assert("TC-Trace-NNNN",
		result == "1.2",
		"operator<<(std::ostream & out, const Trace & trace) writes every element of trace to out in the right order.");
}

//===================================== InputTrace Tests ===================================================

// tests operator<<(std::ostream & out, const InputTrace & trace)
void testInputTraceOutputOperator() {
	// in2String of pl is empty. inTrace contains one element.
	shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
	vector<int> inVec{0};
	InputTrace inTrace{ inVec, pl };
	ostringstream out;
	out << inTrace;
	string result = out.str();
	fsmlib_assert("TC-InputTrace-NNNN",
		result == "0",
		"operator<<(std::ostream & out, const InputTrace & trace) writes every element of trace to out in the right order and with the right name");

	out.str("");
	out.clear();

	// in2String of pl is empty. inTrace contains two elements.
	inVec = { 0, 3 };
	inTrace = { inVec, pl };
	out << inTrace;
	result = out.str();
	fsmlib_assert("TC-InputTrace-NNNN",
		result == "0.3",
		"operator<<(std::ostream & out, const InputTrace & trace) writes every element of trace to out in the right order and with the right name");

	out.str("");
	out.clear();

	// in2String of pl contains one element. inTrace contains one element.
	vector<string> in2String{ "e0" };
	vector<string> out2String{ };
	vector<string> state2String{ };
	pl = make_shared<FsmPresentationLayer>(in2String, out2String, state2String);
	inVec = { 0 };
	inTrace = { inVec, pl };
	out << inTrace;
	result = out.str();
	fsmlib_assert("TC-InputTrace-NNNN",
		result == "e0",
		"operator<<(std::ostream & out, const InputTrace & trace) writes every element of trace to out in the right order and with the right name");

	out.str("");
	out.clear();

	// in2String contains one element. inTrace contains one element >= in2String.size()
	inVec = { 1 };
	inTrace = { inVec, pl };
	out << inTrace;
	result = out.str();
	fsmlib_assert("TC-InputTrace-NNNN",
		result == "1",
		"operator<<(std::ostream & out, const InputTrace & trace) writes every element of trace to out in the right order and with the right name");

	out.str("");
	out.clear();

	// in2String contains one element. inTrace contains two elements. One elements is smaller than in2String.size() and one is equal to in2String.size().
	inVec = { 0,1 };
	inTrace = { inVec, pl };
	out << inTrace;
	result = out.str();
	fsmlib_assert("TC-InputTrace-NNNN",
		result == "e0.1",
		"operator<<(std::ostream & out, const InputTrace & trace) writes every element of trace to out in the right order and with the right name");

	out.str("");
	out.clear();

	// in2String contains two elements. inTrace contains three elements. All elements are smaller than in2String.size().
	in2String = { "e0", "e1" };
	pl = make_shared<FsmPresentationLayer>(in2String, out2String, state2String);
	inVec = { 0,1,0 };
	inTrace = { inVec, pl };
	out << inTrace;
	result = out.str();
	fsmlib_assert("TC-InputTrace-NNNN",
		result == "e0.e1.e0",
		"operator<<(std::ostream & out, const InputTrace & trace) writes every element of trace to out in the right order and with the right name");

	out.str("");
	out.clear();


	// in2String contains two elements. inTrace contains three elements. Two elements are smaller than in2String.size(). One element is equal to this size.
	inVec = { 1,2,1 };
	inTrace = { inVec, pl };
	out << inTrace;
	result = out.str();
	fsmlib_assert("TC-InputTrace-NNNN",
		result == "e1.2.e1",
		"operator<<(std::ostream & out, const InputTrace & trace) writes every element of trace to out in the right order and with the right name");

	out.str("");
	out.clear();
}

//===================================== OutputTrace Tests ===================================================

// tests operator<<(std::ostream & out, const OutputTrace & trace)
void testOutputTraceOutputOperator() {
	// out2String of pl is empty. outTrace contains one element.
	shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
	vector<int> outVec{ 0 };
	OutputTrace outTrace{ outVec, pl };
	ostringstream out;
	out << outTrace;
	string result = out.str();
	fsmlib_assert("TC-OutputTrace-NNNN",
		result == "0",
		"operator<<(std::ostream & out, const OutputTrace & trace) writes every element of trace to out in the right order and with the right name");

	out.str("");
	out.clear();

	// out2String of pl is empty. outTrace contains two elements.
	outVec = { 0, 3 };
	outTrace = { outVec, pl };
	out << outTrace;
	result = out.str();
	fsmlib_assert("TC-OutputTrace-NNNN",
		result == "0.3",
		"operator<<(std::ostream & out, const OutputTrace & trace) writes every element of trace to out in the right order and with the right name");

	out.str("");
	out.clear();

	// out2String of pl contains one element. outTrace contains one element.
	vector<string> in2String{};
	vector<string> out2String{ "e0" };
	vector<string> state2String{};
	pl = make_shared<FsmPresentationLayer>(in2String, out2String, state2String);
	outVec = { 0 };
	outTrace = { outVec, pl };
	out << outTrace;
	result = out.str();
	fsmlib_assert("TC-OutputTrace-NNNN",
		result == "e0",
		"operator<<(std::ostream & out, const OutputTrace & trace) writes every element of trace to out in the right order and with the right name");

	out.str("");
	out.clear();

	// out2String contains one element. outTrace contains one element >= out2String.size()
	outVec = { 1 };
	outTrace = { outVec, pl };
	out << outTrace;
	result = out.str();
	fsmlib_assert("TC-OutputTrace-NNNN",
		result == "1",
		"operator<<(std::ostream & out, const OutputTrace & trace) writes every element of trace to out in the right order and with the right name");

	out.str("");
	out.clear();

	// out2String contains one element. outTrace contains two elements. One elements is smaller than out2String.size() and one is equal to out2String.size().
	outVec = { 0,1 };
	outTrace = { outVec, pl };
	out << outTrace;
	result = out.str();
	fsmlib_assert("TC-OutputTrace-NNNN",
		result == "e0.1",
		"operator<<(std::ostream & out, const OutputTrace & trace) writes every element of trace to out in the right order and with the right name");

	out.str("");
	out.clear();

	// out2String contains two elements. outTrace contains three elements. All elements are smaller than out2String.size().
	out2String = { "e0", "e1" };
	pl = make_shared<FsmPresentationLayer>(in2String, out2String, state2String);
	outVec = { 0,1,0 };
	outTrace = { outVec, pl };
	out << outTrace;
	result = out.str();
	fsmlib_assert("TC-OutputTrace-NNNN",
		result == "e0.e1.e0",
		"operator<<(std::ostream & out, const OutputTrace & trace) writes every element of trace to out in the right order and with the right name");

	out.str("");
	out.clear();


	// out2String contains two elements. outTrace contains three elements. Two elements are smaller than out2String.size(). One element is equal to this size.
	outVec = { 1,2,1 };
	outTrace = { outVec, pl };
	out << outTrace;
	result = out.str();
	fsmlib_assert("TC-OutputTrace-NNNN",
		result == "e1.2.e1",
		"operator<<(std::ostream & out, const OutputTrace & trace) writes every element of trace to out in the right order and with the right name");

	out.str("");
	out.clear();
}

//===================================== OutputTree Tests ===================================================

// tests OutputTree::contains(OutputTree& ot)
// Negative Case.
void testOutputTreeContainsNegative() {
	// inputTrace of this Tree is empty. otherInputTrace of otherTree isn't empty.
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		std::shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{};
		InputTrace inputTrace(inVec, pl);
		OutputTree tree(root, inputTrace, pl);

		shared_ptr<TreeNode> otherRoot = make_shared<TreeNode>();
		vector<int> otherInVec{1};
		InputTrace otherInputTrace(otherInVec, pl);
		OutputTree otherTree(otherRoot, otherInputTrace, pl);

		fsmlib_assert("TC-OutputTree-NNNN",
			not tree.contains(otherTree),
			"OutputTree::contains(OutputTree& ot) returns false if the InputTraces differ.");
	}

	// inputTrace of this Tree isn't empty. otherInputTrace of otherTree is empty.
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		std::shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{ 1 };
		InputTrace inputTrace(inVec, pl);
		OutputTree tree(root, inputTrace, pl);

		shared_ptr<TreeNode> otherRoot = make_shared<TreeNode>();
		vector<int> otherInVec{ };
		InputTrace otherInputTrace(otherInVec, pl);
		OutputTree otherTree(otherRoot, otherInputTrace, pl);

		fsmlib_assert("TC-OutputTree-NNNN",
			not tree.contains(otherTree),
			"OutputTree::contains(OutputTree& ot) returns false if the InputTraces differ.");
	}

	// inputTraces both aren't empty. inputTraces are unequal. 
	// Both Trees contain the same output trace.
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		std::shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{ 1 };
		InputTrace inputTrace(inVec, pl);
		OutputTree tree(root, inputTrace, pl);
		vector<int> outVec{ 1 };
		tree.addToRoot(outVec);

		shared_ptr<TreeNode> otherRoot = make_shared<TreeNode>();
		vector<int> otherInVec{1,2};
		InputTrace otherInputTrace(otherInVec, pl);
		OutputTree otherTree(otherRoot, otherInputTrace, pl);
		vector<int> otherOutVec{ 1 };
		otherTree.addToRoot(otherOutVec);

		fsmlib_assert("TC-OutputTree-NNNN",
			not tree.contains(otherTree),
			"OutputTree::contains(OutputTree& ot) returns false if the InputTraces differ.");
	}

	// inputTraces are equal (both empty).
	// tree contains empty trace. otherTree contains one output trace which is not empty.
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		std::shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{  };
		InputTrace inputTrace(inVec, pl);
		OutputTree tree(root, inputTrace, pl);

		shared_ptr<TreeNode> otherRoot = make_shared<TreeNode>();
		vector<int> otherInVec{  };
		InputTrace otherInputTrace(otherInVec, pl);
		OutputTree otherTree(otherRoot, otherInputTrace, pl);
		vector<int> otherOutVec{ 1 };
		otherTree.addToRoot(otherOutVec);

		fsmlib_assert("TC-OutputTree-NNNN",
			not tree.contains(otherTree),
			"OutputTree::contains(OutputTree& ot) returns false if otherTree contains an output trace which is not contained in tree.");
	}

	// inputTraces are equal (both empty).
	// tree contains only output trace [2]. otherTree contains only output trace [1].
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		std::shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{};
		InputTrace inputTrace(inVec, pl);
		OutputTree tree(root, inputTrace, pl);
		vector<int> outVec{ 2 };
		tree.addToRoot(outVec);

		shared_ptr<TreeNode> otherRoot = make_shared<TreeNode>();
		vector<int> otherInVec{};
		InputTrace otherInputTrace(otherInVec, pl);
		OutputTree otherTree(otherRoot, otherInputTrace, pl);
		vector<int> otherOutVec{ 1 };
		otherTree.addToRoot(otherOutVec);

		fsmlib_assert("TC-OutputTree-NNNN",
			not tree.contains(otherTree),
			"OutputTree::contains(OutputTree& ot) returns false if otherTree contains an output trace which is not contained in tree.");
	}

	// inputTraces are equal.
	// tree contains only output trace [1,2]. otherTree contains only output trace [1] (prefix)).
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		std::shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{1, 2};
		InputTrace inputTrace(inVec, pl);
		OutputTree tree(root, inputTrace, pl);
		vector<int> outVec{ 1, 2 };
		tree.addToRoot(outVec);

		shared_ptr<TreeNode> otherRoot = make_shared<TreeNode>();
		vector<int> otherInVec{ 1, 2 };
		InputTrace otherInputTrace(otherInVec, pl);
		OutputTree otherTree(otherRoot, otherInputTrace, pl);
		vector<int> otherOutVec{ 1 };
		otherTree.addToRoot(otherOutVec);

		fsmlib_assert("TC-OutputTree-NNNN",
			not tree.contains(otherTree),
			"OutputTree::contains(OutputTree& ot) returns false if otherTree contains an output trace which is not contained in tree.");
	}

	// inputTraces are equal.
	// tree contains output traces [1,2] and [2,3]. otherTree contains output traces [1,2] and [2,2] (they share one Trace)).
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		std::shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{ 1, 2 };
		InputTrace inputTrace(inVec, pl);
		OutputTree tree(root, inputTrace, pl);
		vector<int> outVec1{ 1, 2 };
		tree.addToRoot(outVec1);
		vector<int> outVec2{ 2, 3 };
		tree.addToRoot(outVec2);

		shared_ptr<TreeNode> otherRoot = make_shared<TreeNode>();
		vector<int> otherInVec{ 1, 2 };
		InputTrace otherInputTrace(otherInVec, pl);
		OutputTree otherTree(otherRoot, otherInputTrace, pl);
		vector<int> otherOutVec1{ 1, 2 };
		otherTree.addToRoot(otherOutVec1);
		vector<int> otherOutVec2{ 2, 2 };
		otherTree.addToRoot(otherOutVec2);

		fsmlib_assert("TC-OutputTree-NNNN",
			not tree.contains(otherTree),
			"OutputTree::contains(OutputTree& ot) returns false if otherTree contains an output trace which is not contained in tree.");
	}

	// inputTraces are equal.
	// tree contains output trace [1,2]. otherTree contains output trace [1,2,1].
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		std::shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{ 1, 2 };
		InputTrace inputTrace(inVec, pl);
		OutputTree tree(root, inputTrace, pl);
		vector<int> outVec{ 1, 2 };
		tree.addToRoot(outVec);

		shared_ptr<TreeNode> otherRoot = make_shared<TreeNode>();
		vector<int> otherInVec{ 1, 2 };
		InputTrace otherInputTrace(otherInVec, pl);
		OutputTree otherTree(otherRoot, otherInputTrace, pl);
		vector<int> otherOutVec{ 1, 2, 1 };
		otherTree.addToRoot(otherOutVec);

		fsmlib_assert("TC-OutputTree-NNNN",
			not tree.contains(otherTree),
			"OutputTree::contains(OutputTree& ot) returns false if otherTree contains an output trace which is not contained in tree.");
	}

	// inputTraces are equal.
	// tree contains output trace [0]. otherTree contains only empty trace [].
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		std::shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{ 1 };
		InputTrace inputTrace(inVec, pl);
		OutputTree tree(root, inputTrace, pl);
		vector<int> outVec{ 0 };
		tree.addToRoot(outVec);

		shared_ptr<TreeNode> otherRoot = make_shared<TreeNode>();
		vector<int> otherInVec{ 1 };
		InputTrace otherInputTrace(otherInVec, pl);
		OutputTree otherTree(otherRoot, otherInputTrace, pl);

		fsmlib_assert("TC-OutputTree-NNNN",
			not tree.contains(otherTree),
			"OutputTree::contains(OutputTree& ot) returns false if otherTree contains an output trace which is not contained in tree.");
	}
}

// tests OutputTree::contains(OutputTree& ot)
// Positive Case.
void testOutputTreeContainsPositive() {
	// all Traces are empty.
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		std::shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{};
		InputTrace inputTrace(inVec, pl);
		OutputTree tree(root, inputTrace, pl);

		shared_ptr<TreeNode> otherRoot = make_shared<TreeNode>();
		vector<int> otherInVec{};
		InputTrace otherInputTrace(otherInVec, pl);
		OutputTree otherTree(otherRoot, otherInputTrace, pl);

		fsmlib_assert("TC-OutputTree-NNNN",
			tree.contains(otherTree),
			"OutputTree::contains(OutputTree& ot) returns true if the InputTraces are the same and each output trace contained in otherTree is "
			"also contained in tree.");
	}

	// inputTraces are equal.
	// tree contains only output trace [0]. otherTree contains only output trace [0].
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		std::shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{ 1 };
		InputTrace inputTrace(inVec, pl);
		OutputTree tree(root, inputTrace, pl);
		vector<int> outVec{ 0 };
		tree.addToRoot(outVec);

		shared_ptr<TreeNode> otherRoot = make_shared<TreeNode>();
		vector<int> otherInVec{ 1 };
		InputTrace otherInputTrace(otherInVec, pl);
		OutputTree otherTree(otherRoot, otherInputTrace, pl);
		vector<int> otherOutVec{ 0 };
		otherTree.addToRoot(otherOutVec);

		fsmlib_assert("TC-OutputTree-NNNN",
			tree.contains(otherTree),
			"OutputTree::contains(OutputTree& ot) returns true if the InputTraces are the same and each output trace contained in otherTree is "
			"also contained in tree.");
	}

	// inputTraces are equal.
	// tree contains output traces [0,0] and [0,2]. otherTree contains only output trace [0,2].
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		std::shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{ 1,2 };
		InputTrace inputTrace(inVec, pl);
		OutputTree tree(root, inputTrace, pl);
		vector<int> outVec1{ 0,0 };
		tree.addToRoot(outVec1);
		vector<int> outVec2{ 0,2 };
		tree.addToRoot(outVec2);

		shared_ptr<TreeNode> otherRoot = make_shared<TreeNode>();
		vector<int> otherInVec{ 1,2 };
		InputTrace otherInputTrace(otherInVec, pl);
		OutputTree otherTree(otherRoot, otherInputTrace, pl);
		vector<int> otherOutVec{ 0,2 };
		otherTree.addToRoot(otherOutVec);

		fsmlib_assert("TC-OutputTree-NNNN",
			tree.contains(otherTree),
			"OutputTree::contains(OutputTree& ot) returns true if the InputTraces are the same and each output trace contained in otherTree is "
			"also contained in tree.");
	}

	// inputTraces are equal.
	// tree contains output traces [0,0] and [0,2]. otherTree contains only output trace [0,0].
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		std::shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{ 1,2 };
		InputTrace inputTrace(inVec, pl);
		OutputTree tree(root, inputTrace, pl);
		vector<int> outVec1{ 0,0 };
		tree.addToRoot(outVec1);
		vector<int> outVec2{ 0,2 };
		tree.addToRoot(outVec2);

		shared_ptr<TreeNode> otherRoot = make_shared<TreeNode>();
		vector<int> otherInVec{ 1,2 };
		InputTrace otherInputTrace(otherInVec, pl);
		OutputTree otherTree(otherRoot, otherInputTrace, pl);
		vector<int> otherOutVec{ 0,0 };
		otherTree.addToRoot(otherOutVec);

		fsmlib_assert("TC-OutputTree-NNNN",
			tree.contains(otherTree),
			"OutputTree::contains(OutputTree& ot) returns true if the InputTraces are the same and each output trace contained in otherTree is "
			"also contained in tree.");
	}

	// inputTraces are equal.
	// Both Trees are equal and not empty.
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		std::shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{ 1,2 };
		InputTrace inputTrace(inVec, pl);
		OutputTree tree(root, inputTrace, pl);
		vector<int> outVec1{ 0,0 };
		tree.addToRoot(outVec1);
		vector<int> outVec2{ 0,2 };
		tree.addToRoot(outVec2);

		shared_ptr<TreeNode> otherRoot = make_shared<TreeNode>();
		vector<int> otherInVec{ 1,2 };
		InputTrace otherInputTrace(otherInVec, pl);
		OutputTree otherTree(otherRoot, otherInputTrace, pl);
		vector<int> otherOutVec1{ 0,0 };
		otherTree.addToRoot(otherOutVec1);
		vector<int> otherOutVec2{ 0,2 };
		otherTree.addToRoot(otherOutVec2);

		fsmlib_assert("TC-OutputTree-NNNN",
			tree.contains(otherTree),
			"OutputTree::contains(OutputTree& ot) returns true if the InputTraces are the same and each output trace contained in otherTree is "
			"also contained in tree.");
	}
}

// tests OutputTree::toDot(std::ostream& out) 
void testOutputTreeToDot() {
	// root is a leaf. InputTrace is empty.
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		std::shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{ };
		InputTrace inputTrace(inVec, pl);
		OutputTree tree(root, inputTrace, pl);

		std::ostringstream stream;
		tree.toDot(stream);
		string content = stream.str();

		fsmlib_assert("TC-OutputTree-NNNN",
			content.find(" -> ") == string::npos,
			"result of toDot(ostream & out) contains only expected edges");
	}

	// OutputTree contains only one path [1]. InputTrace is [0].
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		std::shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{0};
		InputTrace inputTrace(inVec, pl);
		OutputTree tree(root, inputTrace, pl);
		vector<int> outVec{ 1 };
		tree.addToRoot(outVec); 

		std::ostringstream stream;
		tree.toDot(stream);
		string content = stream.str();

		fsmlib_assert("TC-OutputTree-NNNN",
			content.find("0 -> 1[label = \"0/1\" ];") != string::npos,
			"result of toDot(ostream & out) contains each expected edge");

		fsmlib_assert("TC-OutputTree-NNNN",
			countEdgesInToDotResult(content) == 1,
			"result of toDot(ostream & out) contains only expected edges");
	}

	// OutputTree contains two paths [1] and [2]. InputTrace is [0] (empty). Length of InputTrace is equal to the length of each Trace contained in 
	// OutputTree.
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		std::shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{ 0 };
		InputTrace inputTrace(inVec, pl);
		OutputTree tree(root, inputTrace, pl);
		vector<int> outVec1{ 1 };
		tree.addToRoot(outVec1);
		vector<int> outVec2{ 2 };
		tree.addToRoot(outVec2);

		std::ostringstream stream;
		tree.toDot(stream);
		string content = stream.str();

		fsmlib_assert("TC-OutputTree-NNNN",
			content.find("0 -> 1[label = \"0/1\" ];") != string::npos
			&& content.find("0 -> 2[label = \"0/2\" ];") != string::npos,
			"result of toDot(ostream & out) contains each expected edge");

		fsmlib_assert("TC-OutputTree-NNNN",
			countEdgesInToDotResult(content) == 2,
			"result of toDot(ostream & out) contains only expected edges");
	}

	// OutputTree contains two paths [1] and [2]. InputTrace is [0,1]. Length of InputTrace is greater than the length of at least one Trace
	// contained in OutputTree.
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		std::shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{ 0,1 };
		InputTrace inputTrace(inVec, pl);
		OutputTree tree(root, inputTrace, pl);
		vector<int> outVec1{ 1 };
		tree.addToRoot(outVec1);
		vector<int> outVec2{ 2 };
		tree.addToRoot(outVec2);

		std::ostringstream stream;
		tree.toDot(stream);
		string content = stream.str();

		fsmlib_assert("TC-OutputTree-NNNN",
			content.find("0 -> 1[label = \"0/1\" ];") != string::npos
			&& content.find("0 -> 2[label = \"0/2\" ];") != string::npos,
			"result of toDot(ostream & out) contains each expected edge");

		fsmlib_assert("TC-OutputTree-NNNN",
			countEdgesInToDotResult(content) == 2,
			"result of toDot(ostream & out) contains only expected edges");
	}

	// OutputTree contains paths [1,1], [1,2] and [2]. InputTrace is [0,1]. Length of InputTrace is greater than the length of at least one Trace
	// contained in OutputTree.
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		std::shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{ 0,1 };
		InputTrace inputTrace(inVec, pl);
		OutputTree tree(root, inputTrace, pl);
		vector<int> outVec1{ 1, 1 };
		tree.addToRoot(outVec1);
		vector<int> outVec2{ 1, 2 };
		tree.addToRoot(outVec2);
		vector<int> outVec3{ 2 };
		tree.addToRoot(outVec3);

		std::ostringstream stream;
		tree.toDot(stream);
		string content = stream.str();

		fsmlib_assert("TC-OutputTree-NNNN",
			content.find("0 -> 1[label = \"0/1\" ];") != string::npos
			&& content.find("1 -> 2[label = \"1/1\" ];") != string::npos
			&& content.find("1 -> 3[label = \"1/2\" ];") != string::npos
			&& content.find("0 -> 4[label = \"0/2\" ];") != string::npos,
			"result of toDot(ostream & out) contains each expected edge");

		fsmlib_assert("TC-OutputTree-NNNN",
			countEdgesInToDotResult(content) == 4,
			"result of toDot(ostream & out) contains only expected edges");
	}
}

// tests OutputTree::getOutputTraces()
void testOutputTreeGetOutputTraces() {
	// root is a leaf. InputTrace is empty.
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		std::shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{};
		InputTrace inputTrace(inVec, pl);
		OutputTree tree(root, inputTrace, pl);

		vector<int> outVec{};
		OutputTrace outputTrace(outVec, pl);

		vector<OutputTrace> result = tree.getOutputTraces();

		fsmlib_assert("TC-OutputTree-NNNN",
			result.size() == 1
			&& result.at(0) == outputTrace,
			"OutputTree::getOutputTraces() called on an OutputTree which consists only of a root, returns only an empty OutputTrace.");
	}

	// OutputTree contains two Traces ([1] and [2]).
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		std::shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{0};
		InputTrace inputTrace(inVec, pl);
		OutputTree tree(root, inputTrace, pl);

		vector<int> outVec1{ 1 };
		tree.addToRoot(outVec1);
		OutputTrace outputTrace1(outVec1, pl);
		vector<int> outVec2{ 2 };
		tree.addToRoot(outVec2);
		OutputTrace outputTrace2(outVec2, pl);
		
		vector<OutputTrace> result = tree.getOutputTraces();

		fsmlib_assert("TC-OutputTree-NNNN",
			find(result.cbegin(), result.cend(), outputTrace1) != result.cend()
			&& find(result.cbegin(), result.cend(), outputTrace2) != result.cend(),
			"result of OutputTree::getOutputTraces() contains each expected OutputTrace");
		fsmlib_assert("TC-OutputTree-NNNN",
			result.size() == 2,
			"result of OutputTree::getOutputTraces() contains only expected OutputTraces");
	}

	// OutputTree contains three Traces ([1,1], [1,2] and [2]).
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		std::shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{ 0, 1 };
		InputTrace inputTrace(inVec, pl);
		OutputTree tree(root, inputTrace, pl);		

		vector<int> outVec1{ 1,1 };
		tree.addToRoot(outVec1);
		OutputTrace outputTrace1(outVec1, pl);
		vector<int> outVec2{ 1,2 };
		tree.addToRoot(outVec2);
		OutputTrace outputTrace2(outVec2, pl);
		vector<int> outVec3{ 2 };
		tree.addToRoot(outVec3);
		OutputTrace outputTrace3(outVec3, pl);

		vector<OutputTrace> result = tree.getOutputTraces();

		fsmlib_assert("TC-OutputTree-NNNN",
			find(result.cbegin(), result.cend(), outputTrace1) != result.cend()
			&& find(result.cbegin(), result.cend(), outputTrace2) != result.cend()
			&& find(result.cbegin(), result.cend(), outputTrace3) != result.cend(),
			"result of OutputTree::getOutputTraces() contains each expected OutputTrace");
		fsmlib_assert("TC-OutputTree-NNNN",
			result.size() == 3,
			"result of OutputTree::getOutputTraces() contains only expected OutputTraces");
	}
}

// tests operator<<(std::ostream& out, OutputTree& ot)
void testOutputTreeOutputOperator() {
	// root of OutputTree is a leaf (tree contains only the empty trace). FsmPresentationLayer contains no names.
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		std::shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{  };
		InputTrace inputTrace(inVec, pl);
		OutputTree tree(root, inputTrace, pl);

		ostringstream out;
		out << tree;
		string result = out.str();
		fsmlib_assert("TC-OutputTree-NNNN",
			result == "\n",   
			"operator<<(std::ostream& out, OutputTree& ot) writes each trace contained in OutputTree with the right names to out.");
	}

	// OutputTree contains Traces [0] and [1]. InputTrace = [0].  FsmPresentationLayer contains names for each Input/Output.
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		vector<string> in2String{ "e0", "e1" };
		vector<string> out2String{ "o0", "o1" };
		vector<string> state2String;
		std::shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>(in2String, out2String, state2String);
		vector<int> inVec{ 0 };
		InputTrace inputTrace(inVec, pl);
		OutputTree tree(root, inputTrace, pl);
		vector<int> outVec1{ 0 };
		tree.addToRoot(outVec1);
		vector<int> outVec2{ 1 };
		tree.addToRoot(outVec2);

		ostringstream out;
		out << tree;
		string result = out.str();

		fsmlib_assert("TC-OutputTree-NNNN",
			result.find("(e0/o0)") != string::npos
			&& result.find("(e0/o1)") != string::npos,
			"operator<<(std::ostream& out, OutputTree& ot) writes each trace contained in OutputTree with the right names to out.");
	}

	// OutputTree contains Traces [0,2] and [1]. InputTrace = [0,2].  FsmPresentationLayer contains names for some but not for each Input/Output.
	{
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		vector<string> in2String{ "e0", "e1" };
		vector<string> out2String{ "o0", "o1" };
		vector<string> state2String;
		std::shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>(in2String, out2String, state2String);
		vector<int> inVec{ 0, 2 };
		InputTrace inputTrace(inVec, pl);
		OutputTree tree(root, inputTrace, pl);
		vector<int> outVec1{ 0, 2 };
		tree.addToRoot(outVec1);
		vector<int> outVec2{ 1 };
		tree.addToRoot(outVec2);

		ostringstream out;
		out << tree;
		string result = out.str();

		fsmlib_assert("TC-OutputTree-NNNN",
			result.find("(e0/o0).(2/2)") != string::npos
			&& result.find("(e0/o1)") != string::npos,
			"operator<<(std::ostream& out, OutputTree& ot) writes each trace contained in OutputTree with the right names to out.");
	}
}

//===================================== TestSuite Tests ===================================================

// tests TestSuite::isEquivalentTo(TestSuite& theOtherTs,bool writeOutput)
// Positive Case
// Additional tests for TestSuite::isReductionOf(TestSuite& theOtherTs,bool writeOutput): if ts1 equals ts2 then ts1 is a reduction
// of ts2 and vice versa
void testTestSuiteIsEquivalentToPositive() {
	// empty TestSuites
	{
		TestSuite ts1;
		TestSuite ts2;
		fsmlib_assert("TC-TestSuite-NNNN",
			ts1.isEquivalentTo(ts2)
			&& ts2.isEquivalentTo(ts1),
			"TestSuite::isEquivalentTo(TestSuite& theOtherTs,bool writeOutput) returns true if both TestSuites are empty.");

		fsmlib_assert("TC-TestSuite-NNNN",
			ts1.isReductionOf(ts2)
			&& ts2.isReductionOf(ts1),
			"If TestSuites are equivalent they are also reductions of each other");
	}

	// both TestSuites contain one test case (OutputTree).
	{
		shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{ 0 };
		InputTrace inTrc{ inVec, pl };
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		OutputTree tree{ root, inTrc, pl };
		vector<int> outVec1{ 1 };
		vector<int> outVec2{ 2 };
		tree.addToRoot(outVec1);
		tree.addToRoot(outVec2);
		TestSuite ts;
		ts.push_back(tree);

		shared_ptr<FsmPresentationLayer> o_pl = make_shared<FsmPresentationLayer>();
		vector<int> o_inVec{ 0 };
		InputTrace o_inTrc{ o_inVec, o_pl };
		shared_ptr<TreeNode> o_root = make_shared<TreeNode>();
		OutputTree o_tree{ o_root, o_inTrc, o_pl };
		vector<int> o_outVec1{ 1 };
		vector<int> o_outVec2{ 2 };
		o_tree.addToRoot(o_outVec1);
		o_tree.addToRoot(o_outVec2);
		TestSuite o_ts;
		o_ts.push_back(o_tree);

		fsmlib_assert("TC-TestSuite-NNNN",
			ts.isEquivalentTo(o_ts)
			&& o_ts.isEquivalentTo(ts),
			"TestSuite::isEquivalentTo(TestSuite& theOtherTs,bool writeOutput) returns true if both "
			"TestSuites contain the same OutputTrees in the same order.");

		fsmlib_assert("TC-TestSuite-NNNN",
			ts.isReductionOf(o_ts)
			&& o_ts.isReductionOf(ts),
			"If TestSuites are equivalent they are also reductions of each other");
	}

	// both TestSuites contain two test cases (OutputTrees).
	{
		shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();

		// constructing first tree
		vector<int> inVecTree1{ 0,1 };
		InputTrace inTrcTree1{ inVecTree1, pl };
		shared_ptr<TreeNode> rootTree1 = make_shared<TreeNode>();
		OutputTree tree1{ rootTree1, inTrcTree1, pl };
		vector<int> outVec1Tree1{ 1,1 };
		vector<int> outVec2Tree1{ 1,2 };
		vector<int> outVec3Tree1{ 2,3 };
		tree1.addToRoot(outVec1Tree1);
		tree1.addToRoot(outVec2Tree1);
		tree1.addToRoot(outVec3Tree1);

		// constructing second tree
		vector<int> inVecTree2{ 1,1 };
		InputTrace inTrcTree2{ inVecTree2, pl };
		shared_ptr<TreeNode> rootTree2 = make_shared<TreeNode>();
		OutputTree tree2{ rootTree2, inTrcTree2, pl };
		vector<int> outVec1Tree2{ 3,3 };
		vector<int> outVec2Tree2{ 4,4 };
		tree2.addToRoot(outVec1Tree2);
		tree2.addToRoot(outVec2Tree2);

		TestSuite ts;
		ts.push_back(tree1);
		ts.push_back(tree2);


		//shared_ptr<FsmPresentationLayer> o_pl = make_shared<FsmPresentationLayer>();

		//vector<int> o_inVecTree1{ 0,1 };
		//InputTrace o_inTrcTree1{ o_inVecTree1, o_pl };
		//shared_ptr<TreeNode> o_rootTree1 = make_shared<TreeNode>();
		//OutputTree o_tree1{ o_rootTree1, o_inTrcTree1, o_pl };
		//vector<int> o_outVec1Tree1{ 1,1 };
		//vector<int> o_outVec2Tree1{ 1,2 };
		//vector<int> o_outVec3Tree1{ 2,3 };
		//o_tree1.addToRoot(o_outVec1Tree1);
		//o_tree1.addToRoot(o_outVec2Tree1);
		//o_tree1.addToRoot(o_outVec3Tree1);

		//vector<int> o_inVecTree2{ 0,1 };
		//InputTrace o_inTrcTree2{ o_inVecTree2, o_pl };
		//shared_ptr<TreeNode> o_rootTree2 = make_shared<TreeNode>();
		//OutputTree o_tree2{ o_rootTree2, o_inTrcTree2, o_pl };
		//vector<int> o_outVec1Tree2{ 1,1 };
		//vector<int> o_outVec2Tree2{ 1,2 };
		//vector<int> o_outVec3Tree2{ 2,3 };
		//o_tree2.addToRoot(o_outVec1Tree2);
		//o_tree2.addToRoot(o_outVec2Tree2);
		//o_tree2.addToRoot(o_outVec3Tree2);

		TestSuite o_ts;
		o_ts.push_back(tree1);
		o_ts.push_back(tree2);

		fsmlib_assert("TC-TestSuite-NNNN",
			ts.isEquivalentTo(o_ts)
			&& o_ts.isEquivalentTo(ts),
			"TestSuite::isEquivalentTo(TestSuite& theOtherTs,bool writeOutput) returns true if both "
			"TestSuites contain the same OutputTrees in the same order.");

		fsmlib_assert("TC-TestSuite-NNNN",
			ts.isReductionOf(o_ts)
			&& o_ts.isReductionOf(ts),
			"If TestSuites are equivalent they are also reductions of each other");
	}
}

// tests TestSuite::isEquivalentTo(TestSuite& theOtherTs,bool writeOutput)
// Negative Case
// Additional tests for TestSuite::isReductionOf(TestSuite& theOtherTs,bool writeOutput): if not(ts1isEquivalentTo(ts2) then ts1 and ts2 can't be
// reductions of each other
void testTestSuiteIsEquivalentToNegative() {
	// TestSuites contain different number of OutputTrees.
	{
		shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{ 0 };
		InputTrace inTrc{ inVec, pl };
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		OutputTree tree{ root, inTrc, pl };
		vector<int> outVec1{ 1 };
		vector<int> outVec2{ 2 };
		tree.addToRoot(outVec1);
		tree.addToRoot(outVec2);
		TestSuite ts;
		ts.push_back(tree);

		TestSuite o_ts;

		fsmlib_assert("TC-TestSuite-NNNN",
			not ts.isEquivalentTo(o_ts)
			&& not o_ts.isEquivalentTo(ts),
			"TestSuite::isEquivalentTo(TestSuite& theOtherTs,bool writeOutput) returns false if both TestSuites have different sizes.");

		fsmlib_assert("TC-TestSuite-NNNN",
			not (ts.isReductionOf(o_ts) && o_ts.isReductionOf(ts)),
			"If TestSuites aren't equal they can't be reductions of each other.");
	}

	// both TestSuites have the same number of OutputTrees (1), but they aren't equal. 
	{
		shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{ 0 };
		InputTrace inTrc{ inVec, pl };
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		OutputTree tree{ root, inTrc, pl };
		vector<int> outVec1{ 1 };
		vector<int> outVec2{ 2 };
		tree.addToRoot(outVec1);
		tree.addToRoot(outVec2);
		TestSuite ts;
		ts.push_back(tree);

		shared_ptr<FsmPresentationLayer> o_pl = make_shared<FsmPresentationLayer>();
		vector<int> o_inVec{ 0 };
		InputTrace o_inTrc{ o_inVec, o_pl };
		shared_ptr<TreeNode> o_root = make_shared<TreeNode>();
		OutputTree o_tree{ o_root, o_inTrc, o_pl };
		vector<int> o_outVec1{ 1 };
		o_tree.addToRoot(o_outVec1);
		TestSuite o_ts;
		o_ts.push_back(o_tree);

		fsmlib_assert("TC-TestSuite-NNNN",
			not ts.isEquivalentTo(o_ts)
			&& not o_ts.isEquivalentTo(ts),
			"TestSuite::isEquivalentTo(TestSuite& theOtherTs,bool writeOutput) returns false if one "
			"TestSuite contains an OutputTree that is not contained in the other TestSuite.");

		fsmlib_assert("TC-TestSuite-NNNN",
			not (ts.isReductionOf(o_ts) && o_ts.isReductionOf(ts)),
			"If TestSuites aren't equal they can't be reductions of each other.");
	}

	// both TestSuites contain two test cases (OutputTrees). The TestSuites differ in the second OutputTree (in the InputTraces).
	{
		shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();

		// constructing first tree
		vector<int> inVecTree1{ 0 };
		InputTrace inTrcTree1{ inVecTree1, pl };
		shared_ptr<TreeNode> rootTree1 = make_shared<TreeNode>();
		OutputTree tree1{ rootTree1, inTrcTree1, pl };
		vector<int> outVec1Tree1{ 1 };
		vector<int> outVec2Tree1{ 2 };
		tree1.addToRoot(outVec1Tree1);
		tree1.addToRoot(outVec2Tree1);

		// constructing second tree
		vector<int> inVecTree2{ 0,1 };
		InputTrace inTrcTree2{ inVecTree2, pl };
		shared_ptr<TreeNode> rootTree2 = make_shared<TreeNode>();
		OutputTree tree2{ rootTree2, inTrcTree2, pl };
		vector<int> outVec1Tree2{ 1,3 };
		vector<int> outVec2Tree2{ 2,4 };
		tree2.addToRoot(outVec1Tree2);
		tree2.addToRoot(outVec2Tree2);

		TestSuite ts;
		ts.push_back(tree1);
		ts.push_back(tree2);

		vector<int> o_inVecTree2{ 0,0 };
		InputTrace o_inTrcTree2{ o_inVecTree2, pl };
		shared_ptr<TreeNode> o_rootTree2 = make_shared<TreeNode>();
		OutputTree o_tree2{ o_rootTree2, o_inTrcTree2, pl };
		vector<int> o_outVec1Tree2{ outVec1Tree2 };
		vector<int> o_outVec2Tree2{ outVec2Tree2 };
		o_tree2.addToRoot(o_outVec1Tree2);
		o_tree2.addToRoot(o_outVec2Tree2);

		TestSuite o_ts;
		o_ts.push_back(tree1);
		o_ts.push_back(o_tree2);

		fsmlib_assert("TC-TestSuite-NNNN",
			not ts.isEquivalentTo(o_ts)
			&& not o_ts.isEquivalentTo(ts),
			"TestSuite::isEquivalentTo(TestSuite& theOtherTs,bool writeOutput) returns false if one "
			"TestSuite contains an OutputTree that is not contained in the other TestSuite.");

		fsmlib_assert("TC-TestSuite-NNNN",
			not (ts.isReductionOf(o_ts) && o_ts.isReductionOf(ts)),
			"If TestSuites aren't equal they can't be reductions of each other.");
	}

	// both TestSuites contain the same two test cases (OutputTrees) in different order.
	{
		shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();

		// constructing first tree
		vector<int> inVecTree1{ 0 };
		InputTrace inTrcTree1{ inVecTree1, pl };
		shared_ptr<TreeNode> rootTree1 = make_shared<TreeNode>();
		OutputTree tree1{ rootTree1, inTrcTree1, pl };
		vector<int> outVec1Tree1{ 1 };
		vector<int> outVec2Tree1{ 2 };
		tree1.addToRoot(outVec1Tree1);
		tree1.addToRoot(outVec2Tree1);

		// constructing second tree
		vector<int> inVecTree2{ 0,1 };
		InputTrace inTrcTree2{ inVecTree2, pl };
		shared_ptr<TreeNode> rootTree2 = make_shared<TreeNode>();
		OutputTree tree2{ rootTree2, inTrcTree2, pl };
		vector<int> outVec1Tree2{ 1,3 };
		vector<int> outVec2Tree2{ 2,4 };
		tree2.addToRoot(outVec1Tree2);
		tree2.addToRoot(outVec2Tree2);

		TestSuite ts;
		ts.push_back(tree1);
		ts.push_back(tree2);

		TestSuite o_ts;
		o_ts.push_back(tree2);
		o_ts.push_back(tree1);

		fsmlib_assert("TC-TestSuite-NNNN",
			not ts.isEquivalentTo(o_ts)
			&& not o_ts.isEquivalentTo(ts),
			"TestSuite::isEquivalentTo(TestSuite& theOtherTs,bool writeOutput) returns false if one "
			"TestSuite contains an OutputTree that is not contained in the other TestSuite.");

		fsmlib_assert("TC-TestSuite-NNNN",
			not (ts.isReductionOf(o_ts) && o_ts.isReductionOf(ts)),
			"If TestSuites aren't equal they can't be reductions of each other.");
	}
}

// tests TestSuite::isReductionOf(TestSuite& theOtherTs, bool writeOutput)
// Negative case.
void testTestSuiteIsReductionOfNegative() {
	// TestSuites have different sizes.
	{
		TestSuite ts;

		shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{ 0 };
		InputTrace inTrc{ inVec, pl };
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		OutputTree tree{ root, inTrc, pl };
		vector<int> outVec1{ 1 };
		vector<int> outVec2{ 2 };
		tree.addToRoot(outVec1);
		tree.addToRoot(outVec2);

		TestSuite o_ts;
		o_ts.push_back(tree);

		fsmlib_assert("TC-TestSuite-NNNN",
			not ts.isReductionOf(o_ts),
			"TestSuite::isReductionOf(TestSuite& theOtherTs, bool writeOutput) returns false if both TestSuites have different sizes.");
	}

	// Both TestSuites contain only one OutputTree. The OutputTree of ts contains a path that is not contained in the OutputTree of o_ts.
	{		
		shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();
		vector<int> inVec{ 0 };
		InputTrace inTrc{ inVec, pl };
		shared_ptr<TreeNode> root = make_shared<TreeNode>();
		OutputTree tree{ root, inTrc, pl };
		vector<int> outVec1{ 1 };
		vector<int> outVec2{ 2 };
		tree.addToRoot(outVec1);
		tree.addToRoot(outVec2);

		TestSuite ts;
		ts.push_back(tree);

		vector<int> o_inVec{ 0 };
		InputTrace o_inTrc{ o_inVec, pl };
		shared_ptr<TreeNode> o_root = make_shared<TreeNode>();
		OutputTree o_tree{ o_root, o_inTrc, pl };
		vector<int> o_outVec1{ 1 };
		o_tree.addToRoot(o_outVec1);

		TestSuite o_ts;
		o_ts.push_back(o_tree);

		fsmlib_assert("TC-TestSuite-NNNN",
			not ts.isReductionOf(o_ts),
			"ts1.isReductionOf(ts2) returns false if ts1[i] contains a path that is not contained in ts2[i], for any 0 <= i < size.");
	}

	// Both TestSuites contain two OutputTrees. Two corresponding OutputTrees have different InputTraces.
	{
		shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();

		// constructing first tree
		vector<int> inVecTree1{ 0 };
		InputTrace inTrcTree1{ inVecTree1, pl };
		shared_ptr<TreeNode> rootTree1 = make_shared<TreeNode>();
		OutputTree tree1{ rootTree1, inTrcTree1, pl };
		vector<int> outVec1Tree1{ 1 };
		vector<int> outVec2Tree1{ 2 };
		tree1.addToRoot(outVec1Tree1);
		tree1.addToRoot(outVec2Tree1);

		// constructing second tree
		vector<int> inVecTree2{ 1,1 };
		InputTrace inTrcTree2{ inVecTree2, pl };
		shared_ptr<TreeNode> rootTree2 = make_shared<TreeNode>();
		OutputTree tree2{ rootTree2, inTrcTree2, pl };
		vector<int> outVec1Tree2{ 3,4 };
		tree2.addToRoot(outVec1Tree2);

		TestSuite ts;
		ts.push_back(tree1);
		ts.push_back(tree2);

		vector<int> o_inVecTree2{ 1,2 };
		InputTrace o_inTrcTree2{ o_inVecTree2, pl };
		shared_ptr<TreeNode> o_rootTree2 = make_shared<TreeNode>();
		OutputTree o_tree2{ o_rootTree2, o_inTrcTree2, pl };
		vector<int> o_outVec1Tree2{ outVec1Tree2 };
		o_tree2.addToRoot(o_outVec1Tree2);

		TestSuite o_ts;
		o_ts.push_back(tree1);
		o_ts.push_back(o_tree2);

		fsmlib_assert("TC-TestSuite-NNNN",
			not ts.isReductionOf(o_ts),
			"ts1.isReductionOf(ts2) returns false if ts1[i] and ts2[i] differ in the InputTrace, for any 0 <= i < size.");
	}

	// Both TestSuites contain two OutputTrees. Each OutputTree of ts is a contained in some OutputTree of o_ts, but ts has the wrong order.
	{
		shared_ptr<FsmPresentationLayer> pl = make_shared<FsmPresentationLayer>();

		// constructing first tree
		vector<int> inVecTree1{ 0 };
		InputTrace inTrcTree1{ inVecTree1, pl };
		shared_ptr<TreeNode> rootTree1 = make_shared<TreeNode>();
		OutputTree tree1{ rootTree1, inTrcTree1, pl };
		vector<int> outVec1Tree1{ 1 };
		vector<int> outVec2Tree1{ 2 };
		tree1.addToRoot(outVec1Tree1);
		tree1.addToRoot(outVec2Tree1);

		// constructing second tree
		vector<int> inVecTree2{ 1 };
		InputTrace inTrcTree2{ inVecTree2, pl };
		shared_ptr<TreeNode> rootTree2 = make_shared<TreeNode>();
		OutputTree tree2{ rootTree2, inTrcTree2, pl };
		vector<int> outVec1Tree2{ 3 };
		tree2.addToRoot(outVec1Tree2);

		TestSuite ts;
		ts.push_back(tree1);
		ts.push_back(tree2);

		vector<int> o_inVecTree2{ 1 };
		InputTrace o_inTrcTree2{ o_inVecTree2, pl };
		shared_ptr<TreeNode> o_rootTree2 = make_shared<TreeNode>();
		OutputTree o_tree2{ o_rootTree2, o_inTrcTree2, pl };
		vector<int> o_outVec1Tree2{ 3 };
		vector<int> o_outVec2Tree2{ 4 };
		o_tree2.addToRoot(o_outVec1Tree2);
		o_tree2.addToRoot(o_outVec2Tree2);

		TestSuite o_ts;
		o_ts.push_back(o_tree2);
		o_ts.push_back(tree1);

		fsmlib_assert("TC-TestSuite-NNNN",
			not ts.isReductionOf(o_ts),
			"ts1.isReductionOf(ts2) returns false if ts1[i] contains a path that is not contained in ts2[i], for any 0 <= i < size.");
	}
}

// tests TestSuite::isReductionOf(TestSuite& theOtherTs, bool writeOutput)
// Positive case.
void testTestSuiteIsReductionOfPositive() {

}


int main(int argc, char** argv)
{
    
    
    
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
	testTestSuiteIsReductionOfNegative();

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



