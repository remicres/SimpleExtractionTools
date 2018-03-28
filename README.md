# Simple Extraction Tools
Simple extraction tools remote module developed in the framework of Theia-Geosud

# Applications for the user

## ExtractBand
An application dedicated to extract channels of an image.

## ExtractGeom
An application which extracts an image region from an input vector data. The output image is cut around the vector data and croped on the resulting region, that is the intersection between the vector data region and the entire image region.

## MeanResample
Produces an output image resampled using the local mean of pixels.

## ZonalStatistics
An application dedicated to compute zonal satistics (min, max, mean, standard deviation) of an image for each polygons of the input vector data.

# Stuff for the developper
This remote module of Orféo ToolBox contains some useful filters and stuff for remote sensing image processing. 

## CacheLessLabelImageToVectorData
This mapper produces an output vector data layer. The input pipeline it triggered using streaming to avoid caching the entire bulk of data on the largest possible region of the pipeline output. It's largely inspired from the OTB ImageFileWriter, but uses the LabelImageToVectorData filter to vectorize the label image. Work only on integer images (not vector images!).

## RegionComparator
A set of useful functions, maybe already existing somewhere in the OTB. Images layout, region intersection, coordinates conversion of regions, etc.

## VectorDataToLabelImageCustomFilter
This is the clone of the VectorDataToLabelImageFilter, but this one has one option for burning one given value.
