cmake_minimum_required (VERSION 2.8)

OTB_CREATE_APPLICATION(NAME           ExtractGeom
                       SOURCES        otbExtractGeom.cxx
                       LINK_LIBRARIES OTBCommon)

OTB_CREATE_APPLICATION(NAME           MeanResample
                       SOURCES        otbMeanResample.cxx
                       LINK_LIBRARIES OTBCommon)
