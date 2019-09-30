cmake_minimum_required (VERSION 2.8)

OTB_CREATE_APPLICATION(NAME           ExtractGeom
                       SOURCES        otbExtractGeom.cxx
                       LINK_LIBRARIES ${${otb-module}_LIBRARIES})

OTB_CREATE_APPLICATION(NAME           MeanResample
                       SOURCES        otbMeanResample.cxx
                       LINK_LIBRARIES ${${otb-module}_LIBRARIES})
