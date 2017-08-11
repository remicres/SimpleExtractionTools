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
#include "otbRegionComparator.h"

// ogr
#include "otbOGR.h"

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
    OGREnvelope env;
    otb::ogr::DataSource::Pointer ogrDS;
    ogrDS = otb::ogr::DataSource::New(GetParameterString("shp") ,
        otb::ogr::DataSource::Modes::Read);
    double ulx, uly, lrx, lry;
    bool extentAvailable = true;
    std::string inputProjectionRef = "";
    // First try to get the extent in the metadata
    try
    {
      inputProjectionRef = ogrDS->GetGlobalExtent(ulx,uly,lrx,lry);
    }
    catch(const itk::ExceptionObject&)
    {
      extentAvailable = false;
    }
    // If no extent available force the computation of the extent
    if (!extentAvailable)
      {
      try
      {
        inputProjectionRef = ogrDS->GetGlobalExtent(ulx,uly,lrx,lry,true);
        extentAvailable = true;
      }
      catch(itk::ExceptionObject & err)
      {
        extentAvailable = false;

        otbAppLogFATAL(<<"Failed to retrieve the spatial extent of the dataset "
            "in force mode. The spatial extent is mandatory when "
            "orx, ory, spx and spy parameters are not set, consider "
            "setting them. Error from library: "<<err.GetDescription());
      }
      }
    otb::RegionComparator<FloatVectorImageType, FloatVectorImageType>::RSRegion::PointType rsRoiOrigin;
    rsRoiOrigin[0] = ulx;
    rsRoiOrigin[1] = uly;
    otb::RegionComparator<FloatVectorImageType, FloatVectorImageType>::RSRegion::SizeType rsRoiSize;
    rsRoiSize[0] = lrx - ulx;
    rsRoiSize[1] = lry - uly;
    otb::RegionComparator<FloatVectorImageType, FloatVectorImageType>::RSRegion rsRoi;
    rsRoi.SetOrigin(rsRoiOrigin);
    rsRoi.SetSize(rsRoiSize);

    /* Compute intersecting region */
    otb::RegionComparator<FloatVectorImageType, FloatVectorImageType> comparator;
    comparator.SetImage1(xs);
    FloatVectorImageType::RegionType roi =
        comparator.RSRegionToImageRegion(rsRoi /*vdProperties->GetBoundingRegion()*/);
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
  MaskFilterType::Pointer maskFilter;

};
}
}

OTB_APPLICATION_EXPORT( otb::Wrapper::ExtractGeom )
