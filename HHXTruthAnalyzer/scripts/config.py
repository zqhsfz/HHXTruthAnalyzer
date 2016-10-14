import ROOT
from xAH_config import xAH_config
import sys,os

sys.path.insert(0, os.environ['ROOTCOREBIN']+"/user_scripts/PixelClusterAnalyzer/")

c = xAH_config()

c.setalg("NtupleMaker", {
	                      "m_debug"          : False,
	                      "m_outputFileName" : "TruthNtuple",
	                      "m_outputTreeName" : "TruthTree",
	                    })