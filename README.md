# Simple Extraction Tools
Simple extraction tools remote module developed in the framework of Theia-Geosud

# Application for the user

## MeanResample
Produces an output image resampled using the local mean of pixels.

# Stuff for the developper
This remote module of Orfeo ToolBox contains some useful filters and stuff for remote sensing image processing. 

## CacheLessLabelImageToVectorData
This mapper produces an output vector data layer. The input pipeline it triggered using streaming to avoid caching the entire bulk of data on the largest possible region of the pipeline output. It's largely inspired from the OTB ImageFileWriter, but uses the LabelImageToVectorData filter to vectorize the label image. Work only on integer images (not vector images!).

## RegionComparator
A set of useful functions, maybe already existing somewhere in the OTB. Images layout, region intersection, coordinates conversion of regions, etc.

## VectorDataToLabelImageCustomFilter
This is the clone of the VectorDataToLabelImageFilter, but this one has one option for burning one given value.
