/*=========================================================================

  Copyright (c) IRSTEA. All rights reserved.


     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include "itkFixedArray.h"
#include "itkObjectFactory.h"

// Elevation handler
#include "otbWrapperElevationParametersHandler.h"
#include "otbWrapperApplicationFactory.h"

// Application engine
#include "otbStandardFilterWatcher.h"
#include "itkFixedArray.h"

// Filter
#include "otbMultiChannelExtractROI.h"
#include "otbVectorDataToLabelImageCustomFilter.h"
#include "itkMaskImageFilter.h"
#include "otbVectorDataIntoImageProjectionFilter.h"
#include "otbVectorDataProperties.h"
#include "otbRegionComparator.h"

using namespace std;

namespace otb
{

namespace Wrapper
{

class ExtractGeom : public Application
{
public:
  /** Standard class typedefs. */
  typedef ExtractGeom                        Self;
  typedef Application                   Superclass;
  typedef itk::SmartPointer<Self>       Pointer;
  typedef itk::SmartPointer<const Self> ConstPointer;

  /** Standard macro */
  itkNewMacro(Self);
  itkTypeMacro(ExtractGeom, Application);

  /** Filters */
  typedef otb::MultiChannelExtractROI<FloatVectorImageType::InternalPixelType,
      FloatVectorImageType::InternalPixelType> ExtractFilterType;

  /** mask */
  typedef bool TMaskPixelValueType;
  typedef otb::Image<TMaskPixelValueType, 2> MaskImageType;
  typedef itk::MaskImageFilter<FloatVectorImageType, MaskImageType,
      FloatVectorImageType> MaskFilterType;

  /* vector data filters */
  typedef otb::VectorDataIntoImageProjectionFilter<VectorDataType,
      FloatVectorImageType> VectorDataReprojFilterType;
  typedef otb::VectorDataToLabelImageCustomFilter<VectorDataType,
      MaskImageType> RasteriseFilterType;
  typedef otb::VectorDataProperties<VectorDataType> VectorDataPropertiesType;

  void DoInit()
  {
    SetName("ExtractGeom");
    SetDescription("Perform geometric extraction");

    // Documentation
    SetDocName("ExtractGeom");
    SetDocLongDescription("This application performs geometric extraction");
    SetDocLimitations("None");
    SetDocAuthors("RemiCresson");
    SetDocSeeAlso(" ");

    AddDocTag(Tags::Manip);

    AddParameter(ParameterType_InputImage,   "in",  "Input Image");
    SetParameterDescription("in"," Input image.");

    AddParameter(ParameterType_InputVectorData, "shp", "Input Shapefile" );
    SetParameterDescription("shp","Input Shapefile" );

    AddParameter(ParameterType_OutputImage,  "out",   "Output image");
    SetParameterDescription("out"," Output image.");

    AddRAMParameter();

    // Doc example parameter settings
    SetDocExampleParameterValue("shp", "vecteur.shp");
    SetDocExampleParameterValue("in", "QB_Toulouse_Ortho_XS.tif");
    SetDocExampleParameterValue("out", "QB_Toulouse_Ortho_XS_decoup.tif uint16");




  }

  void DoUpdateParameters()
  {
    // Nothing to do here : all parameters are independent
  }

  void DoExecute()
  {

    // Get inputs
    FloatVectorImageType::Pointer xs = GetParameterImage("in");
    VectorDataType* shp = GetParameterVectorData("shp");

    /* Reproject vector data */
    vdReproj = VectorDataReprojFilterType::New();
    vdReproj->SetInputVectorData(shp);
    vdReproj->SetInputImage(xs);
    vdReproj->Update();

    /* Get VectorData bounding box */
    vdProperties = VectorDataPropertiesType::New();
    vdProperties->SetVectorDataObject(vdReproj->GetOutput());
    vdProperties->ComputeBoundingRegion();

    /* Compute intersecting region */
    otb::RegionComparator<FloatVectorImageType, FloatVectorImageType> comparator;
    comparator.SetImage1(xs);
    FloatVectorImageType::RegionType roi =
        comparator.RSRegionToImageRegion(vdProperties->GetBoundingRegion());
    roi.PadByRadius(1); // avoid extrapolation
    if (!roi.Crop(xs->GetLargestPossibleRegion()))
      {
      otbAppLogFATAL( << " Input VectorData is outside image !" );
      return;
      }

    /* Rasterize vector data */
    rasterizer = RasteriseFilterType::New();
    rasterizer->AddVectorData(vdReproj->GetOutput());
    rasterizer->SetOutputOrigin(xs->GetOrigin());
    rasterizer->SetOutputSpacing(xs->GetSpacing());
    rasterizer->SetOutputSize(
        xs->GetLargestPossibleRegion().GetSize());
    rasterizer->SetBurnMaxValueMode(true);
    rasterizer->SetOutputProjectionRef(
        xs->GetProjectionRef());
    rasterizer->Update();

    /* Mask input image */
    maskFilter = MaskFilterType::New();
    maskFilter->SetInput(xs);
    maskFilter->SetMaskImage(rasterizer->GetOutput());

    /* Extract ROI */
    m_Filter = ExtractFilterType::New();
    m_Filter->SetInput(maskFilter->GetOutput());
    m_Filter->SetExtractionRegion(roi);

    SetParameterOutputImage("out", m_Filter->GetOutput());


  }

  ExtractFilterType::Pointer m_Filter;
  VectorDataReprojFilterType::Pointer vdReproj;
  RasteriseFilterType::Pointer rasterizer;
  VectorDataPropertiesType::Pointer vdProperties;
  MaskFilterType::Pointer maskFilter;

};
}
}

OTB_APPLICATION_EXPORT( otb::Wrapper::ExtractGeom )
