set(DOCUMENTATION "Extraction of subset of remote sensing images")

otb_module(SimpleExtractionTools
  DEPENDS
    OTBCommon
	OTBApplicationEngine
	OTBConversion
	OTBImageBase
  TEST_DEPENDS
    OTBTestKernel
    OTBCommandLine
  DESCRIPTION
    $DOCUMENTATION
)
