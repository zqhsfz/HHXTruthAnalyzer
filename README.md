# HHXTruthAnalyzer
Ntuple maker / analyzer for HHX DAOD_Truth1 samples

First you need to setup cvmfs appropriately at SLAC. Contact me directly if you don't know how to do it.

Then:

1. setupATLAS
2. lsetup -f "python 2.7.4-x86_64-slc6-gcc48" --- This is a very specific hack at SLAC, since somehow the python would not be setup appropriately from RootCore
3. Go to HHXTruthAnalyzer
4. source setup.sh (Only need to do this once when you checkout the repository first time. Then you only need to run "rcSetup")
5. source checkout.sh
6. rc find_package
7. rc compile

To run the code, just do:

python run.py
