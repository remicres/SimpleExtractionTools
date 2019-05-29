/*=========================================================================

  Copyright (c) Remi Cresson (IRSTEA). All rights reserved.


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
#include "otbGeometriesProjectionFilter.h"
#include "otbGeometriesSet.h"

// No-data
#include "otbChangeInformationImageFilter.h"

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

  /** no-data */
  typedef otb::ChangeInformationImageFilter<FloatVectorImageType>     ChangeInfoFilterType;

  typedef otb::GeometriesSet GeometriesType;

  typedef otb::GeometriesProjectionFilter ProjectionFilterType;

  void DoInit()
  {
    SetName("ExtractGeom");
    SetDescription("Perform geometric extraction");

    // Documentation
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

    // Get vector data BBOX
    otb::ogr::DataSource::Pointer vectors =
      otb::ogr::DataSource::New(this->GetParameterString("vec"));
    otb::ogr::DataSource::Pointer reprojVector = vectors;
    GeometriesType::Pointer inputGeomSet;
    ProjectionFilterType::Pointer geometriesProjFilter;
    GeometriesType::Pointer outputGeomSet;
    bool doReproj = true;
    // don't reproject for these cases
    std::string imageProjectionRef = xs->GetProjectionRef();
    FloatVectorImageType::ImageKeywordlistType imageKwl = xs->GetImageKeywordlist();
    std::string vectorProjectionRef = shp->GetProjectionRef();
    if (vectorProjectionRef.empty() ||
        (imageProjectionRef == vectorProjectionRef) ||
        (imageProjectionRef.empty() && imageKwl.GetSize() == 0))
      doReproj = false;

    if (doReproj)
      {
      inputGeomSet = GeometriesType::New(vectors);
      reprojVector = otb::ogr::DataSource::New();
      outputGeomSet = GeometriesType::New(reprojVector);
      // Filter instantiation
      geometriesProjFilter = ProjectionFilterType::New();
      geometriesProjFilter->SetInput(inputGeomSet);
      if (imageProjectionRef.empty())
        {
        geometriesProjFilter->SetOutputKeywordList(imageKwl);
        }
      geometriesProjFilter->SetOutputProjectionRef(imageProjectionRef);
      geometriesProjFilter->SetOutput(outputGeomSet);
      otbAppLogINFO("Reprojecting input vectors...");
      geometriesProjFilter->Update();
      }

    /* Get VectorData bounding box */
    itk::Point<double, 2> ulp_in,  lrp_in;
    bool extentAvailable = true;
    std::string inputProjectionRef = "";

    // First try to get the extent in the metadata
    try
    {
        inputProjectionRef = reprojVector->GetGlobalExtent(ulp_in[0],ulp_in[1],lrp_in[0],lrp_in[1]);
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
            inputProjectionRef = reprojVector->GetGlobalExtent(ulp_in[0],ulp_in[1],lrp_in[0],lrp_in[1],true);
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
    rsRoiOrigin[0] = ulp_in[0] ;
    rsRoiOrigin[1] = ulp_in[1] ;
    otb::RegionComparator<FloatVectorImageType, FloatVectorImageType>::RSRegion::SizeType rsRoiSize;
    rsRoiSize[0] = lrp_in[ 0 ] - rsRoiOrigin[0];
    rsRoiSize[1] = lrp_in[ 1 ] - rsRoiOrigin[1];
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
    m_RasterizeFilter->SetOutputSpacing(xs->GetSignedSpacing());
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

    /* Change no-data value */
    std::vector<bool> flags;
    std::vector<double> values;
    unsigned int nbBands = xs->GetNumberOfComponentsPerPixel();
    flags.resize(nbBands, true);
    values.resize(nbBands, 0.0);
    m_MetaDataChanger = ChangeInfoFilterType::New();
    m_MetaDataChanger->SetInput(m_ExtractFilter->GetOutput());
    m_MetaDataChanger->SetOutputMetaData<std::vector<bool> >(otb::MetaDataKey::NoDataValueAvailable,&flags);
    m_MetaDataChanger->SetOutputMetaData<std::vector<double> >(otb::MetaDataKey::NoDataValue,&values);

    SetParameterOutputImage("out", m_MetaDataChanger->GetOutput());

  }

  ExtractFilterType::Pointer            m_ExtractFilter;
  VectorDataReprojFilterType::Pointer   m_VectorDataReprojectionFilter;
  RasteriseFilterType::Pointer          m_RasterizeFilter;
  MaskFilterType::Pointer               m_MaskFilter;
  ChangeInfoFilterType::Pointer         m_MetaDataChanger;
};
}
}

OTB_APPLICATION_EXPORT( otb::Wrapper::ExtractGeom )
