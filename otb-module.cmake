set(DOCUMENTATION "Extraction of subset of remote sensing images")

otb_module(SimpleExtractionTools
  DEPENDS
    OTBCommon
	OTBApplicationEngine
	OTBConversion
	OTBImageBase
	OTBImageIO
	OTBStatistics
        OTBMosaic
	
  TEST_DEPENDS
    OTBTestKernel
    OTBCommandLine
  DESCRIPTION
    $DOCUMENTATION
)
