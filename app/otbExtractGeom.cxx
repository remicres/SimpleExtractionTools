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

// Projection
#include "otbGenericRSTransform.h"

using namespace std;

namespace otb
{

namespace Wrapper
{

class ExtractGeom : public Application
{
public:
  /** Standard class typedefs. */
  typedef ExtractGeom                         Self;
  typedef Application                         Superclass;
  typedef itk::SmartPointer<Self>             Pointer;
  typedef itk::SmartPointer<const Self>       ConstPointer;

  /** Standard macro */
  itkNewMacro(Self);
  itkTypeMacro(ExtractGeom, Application);

  /** Filters */
  typedef otb::MultiChannelExtractROI<FloatVectorImageType::InternalPixelType,
      FloatVectorImageType::InternalPixelType>                        ExtractFilterType;

  /** mask */
  typedef bool                                                        TMaskPixelValueType;
  typedef otb::Image<TMaskPixelValueType, 2>                          MaskImageType;
  typedef itk::MaskImageFilter<FloatVectorImageType, MaskImageType,
      FloatVectorImageType>                                           MaskFilterType;

  /* vector data filters */
  typedef otb::VectorDataIntoImageProjectionFilter<VectorDataType,
      FloatVectorImageType>                                           VectorDataReprojFilterType;
  typedef otb::VectorDataToLabelImageCustomFilter<VectorDataType,
      MaskImageType>                                                  RasteriseFilterType;

  /** projection */
  typedef otb::GenericRSTransform<>                                   RSTransformType;
  typedef otb::GenericMapProjection<otb::TransformDirection::FORWARD> MapProjectionType;
  typedef RSTransformType::InputPointType                             Point3DType;

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

    AddParameter(ParameterType_InputVectorData, "vec", "Input vector" );
    SetParameterDescription("vec","Input vector" );

    AddParameter(ParameterType_OutputImage,  "out",   "Output image");
    SetParameterDescription("out"," Output image.");

    AddRAMParameter();

    // Doc example parameter settings
    SetDocExampleParameterValue("vec", "vecteur.shp");
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
    VectorDataType* shp = GetParameterVectorData("vec");

    /* Reproject vector data */
    m_VectorDataReprojectionFilter = VectorDataReprojFilterType::New();
    m_VectorDataReprojectionFilter->SetInputVectorData(shp);
    m_VectorDataReprojectionFilter->SetInputImage(xs);
    m_VectorDataReprojectionFilter->Update();

    /* Get VectorData bounding box */
    OGREnvelope env;
    otb::ogr::DataSource::Pointer ogrDS;
    ogrDS = otb::ogr::DataSource::New(GetParameterString("vec") ,
        otb::ogr::DataSource::Modes::Read);
    itk::Point<double, 2> ulp_in,  lrp_in;
    bool extentAvailable = true;
    std::string inputProjectionRef = "";

    // First try to get the extent in the metadata
    try
    {
        inputProjectionRef = ogrDS->GetGlobalExtent(ulp_in[0],ulp_in[1],lrp_in[0],lrp_in[1]);
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
            inputProjectionRef = ogrDS->GetGlobalExtent(ulp_in[0],ulp_in[1],lrp_in[0],lrp_in[1],true);
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

    // Reproject region
    RSTransformType::Pointer rsTransform = RSTransformType::New();
    rsTransform->SetInputProjectionRef(shp->GetProjectionRef());
    rsTransform->SetOutputKeywordList(xs->GetImageKeywordlist());
    rsTransform->SetOutputProjectionRef(xs->GetProjectionRef());
    rsTransform->InstantiateTransform();
    itk::Point<double, 2> ulp_out , lrp_out;
    ulp_out = rsTransform->TransformPoint(ulp_in);
    lrp_out = rsTransform->TransformPoint(lrp_in);
    otb::RegionComparator<FloatVectorImageType, FloatVectorImageType>::RSRegion::PointType rsRoiOrigin;
    rsRoiOrigin[0] = ulp_out[0];
    rsRoiOrigin[1] = ulp_out[1];
    otb::RegionComparator<FloatVectorImageType, FloatVectorImageType>::RSRegion::SizeType rsRoiSize;
    rsRoiSize[0] = lrp_out[ 0 ] - ulp_out[0];
    rsRoiSize[1] = lrp_out[ 1 ] - ulp_out[1];
    otb::RegionComparator<FloatVectorImageType, FloatVectorImageType>::RSRegion rsRoi;
    rsRoi.SetOrigin(rsRoiOrigin);
    rsRoi.SetSize(rsRoiSize);

    /* Compute intersecting region */
    otb::RegionComparator<FloatVectorImageType, FloatVectorImageType> comparator;
    comparator.SetImage1(xs);
    FloatVectorImageType::RegionType roi = comparator.RSRegionToImageRegion(rsRoi);
    roi.PadByRadius(1); // avoid extrapolation
    if (!roi.Crop(xs->GetLargestPossibleRegion()))
      {
        otbAppLogFATAL( << " Input VectorData is outside image !" );
        return;
      }

    /* Rasterize vector data */
    m_RasterizeFilter = RasteriseFilterType::New();
    m_RasterizeFilter->AddVectorData(m_VectorDataReprojectionFilter->GetOutput());
    m_RasterizeFilter->SetOutputOrigin(xs->GetOrigin());
    m_RasterizeFilter->SetOutputSpacing(xs->GetSpacing());
    m_RasterizeFilter->SetOutputSize(xs->GetLargestPossibleRegion().GetSize());
    m_RasterizeFilter->SetBurnMaxValueMode(true);
    m_RasterizeFilter->SetOutputProjectionRef(xs->GetProjectionRef());
    m_RasterizeFilter->Update();

    /* Mask input image */
    m_MaskFilter = MaskFilterType::New();
    m_MaskFilter->SetInput(xs);
    m_MaskFilter->SetMaskImage(m_RasterizeFilter->GetOutput());

    /* Extract ROI */
    m_ExtractFilter = ExtractFilterType::New();
    m_ExtractFilter->SetInput(m_MaskFilter->GetOutput());
    m_ExtractFilter->SetExtractionRegion(roi);

    SetParameterOutputImage("out", m_ExtractFilter->GetOutput());

  }

  ExtractFilterType::Pointer            m_ExtractFilter;
  VectorDataReprojFilterType::Pointer   m_VectorDataReprojectionFilter;
  RasteriseFilterType::Pointer          m_RasterizeFilter;
  MaskFilterType::Pointer               m_MaskFilter;

};
}
}

OTB_APPLICATION_EXPORT( otb::Wrapper::ExtractGeom )
