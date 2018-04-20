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

// Filters
#include "otbVectorDataToLabelImageFilter.h"
#include "otbVectorDataIntoImageProjectionFilter.h"
#include "otbStreamingStatisticsMapFromLabelImageFilter.h"

namespace otb
{

namespace Wrapper
{

class ZonalStatistics : public Application
{
public:
  /** Standard class typedefs. */
  typedef ZonalStatistics                        Self;
  typedef Application                   Superclass;
  typedef itk::SmartPointer<Self>       Pointer;
  typedef itk::SmartPointer<const Self> ConstPointer;


  /* vector data filters */
  typedef UInt32ImageType                                             LabelImageType;
  typedef LabelImageType::ValueType                                   LabelValueType;
  typedef otb::VectorData<double, 2>                                  VectorDataType;
  typedef otb::VectorDataIntoImageProjectionFilter<VectorDataType,
      FloatVectorImageType>                                           VectorDataReprojFilterType;
  typedef otb::VectorDataToLabelImageFilter<VectorDataType,
      LabelImageType>                                                 RasterizeFilterType;
  typedef VectorDataType::DataTreeType                                DataTreeType;
  typedef itk::PreOrderTreeIterator<DataTreeType>                     TreeIteratorType;
  typedef VectorDataType::DataNodeType                                DataNodeType;
  typedef DataNodeType::PolygonListPointerType                        PolygonListPointerType;

  /** Statistics */
  typedef otb::StreamingStatisticsMapFromLabelImageFilter<FloatVectorImageType,
      LabelImageType>                                                 StatsFilterType;

  /** Standard macro */
  itkNewMacro(Self);
  itkTypeMacro(ZonalStatistics, Application);

  void DoInit()
  {

    SetName("ZonalStatistics");
    SetDescription("Computes zonal statistics");

    // Documentation
    SetDocName("ZonalStatistics");
    SetDocLongDescription("This application zonal statistics");
    SetDocLimitations("None");
    SetDocAuthors("Remi Cresson");

    AddDocTag(Tags::Manip);

    // Inputs
    AddParameter(ParameterType_InputImage,   "in",  "Input Image");
    AddParameter(ParameterType_InputVectorData, "vec", "Input vector data");

    // No data value
    AddParameter(ParameterType_Float, "nodata", "No-data value");
    MandatoryOff("nodata");

    // reprojection
    AddParameter(ParameterType_Bool, "reproject", "Reproject the input vector");

    // Output
    AddParameter(ParameterType_OutputVectorData, "out",   "Output vector data");

    AddRAMParameter();

    // Doc example parameter settings
    SetDocExampleParameterValue("in", "input.tif");
    SetDocExampleParameterValue("vec", "terrain.shp");
    SetDocExampleParameterValue("out", "terain_with_stats.shp");


  }

  void DoUpdateParameters()
  {
    // Nothing to do here : all parameters are independent
  }

  // Create a string of the kind "prefix_i"
  std::string CreateFieldName(std::string prefix, unsigned int i)
  {
    std::stringstream ss;
    ss << prefix << "_" << i;
    return ss.str();
  }

  FloatVectorImageType::PixelType NullPixel(FloatVectorImageType::Pointer & img)
  {
  const unsigned int nBands = img->GetNumberOfComponentsPerPixel();
  FloatVectorImageType::PixelType pix;
  pix.SetSize(nBands);
  pix.Fill(0);
  return pix;
  }

  void DoExecute()
  {

    // Get inputs
    FloatVectorImageType::Pointer img = GetParameterImage("in");
    VectorDataType* shp = GetParameterVectorData("vec");
    const bool has_nodata = HasValue("nodata");
    FloatVectorImageType::InternalPixelType m_InputNoData = 0;
    if (has_nodata)
      {
      m_InputNoData = GetParameterFloat("nodata");
      otbAppLogINFO("Using user no-data value: " << m_InputNoData);
      }

    // Reproject vector data
    if (GetParameterInt("reproject") != 0)
      {
      otbAppLogINFO("Reproject vector data");
      m_VectorDataReprojectionFilter = VectorDataReprojFilterType::New();
      m_VectorDataReprojectionFilter->SetInputVectorData(shp);
      m_VectorDataReprojectionFilter->SetInputImage(img);
      AddProcess(m_VectorDataReprojectionFilter, "Reproject vector data");
      m_VectorDataReprojectionFilter->Update();
      m_VectorDataSrc = m_VectorDataReprojectionFilter->GetOutput();
      }
    else
      {
      m_VectorDataSrc = shp;
      }

    // Internal no-data value
    const LabelValueType intNoData = itk::NumericTraits<LabelValueType>::max();

    // Rasterize vector data
    m_RasterizeFilter = RasterizeFilterType::New();
    m_RasterizeFilter->AddVectorData(m_VectorDataSrc);
    m_RasterizeFilter->SetOutputOrigin(img->GetOrigin());
    m_RasterizeFilter->SetOutputSpacing(img->GetSignedSpacing());
    m_RasterizeFilter->SetOutputSize(img->GetLargestPossibleRegion().GetSize());
    m_RasterizeFilter->SetOutputProjectionRef(img->GetProjectionRef());
    m_RasterizeFilter->SetBurnAttribute("________");
    m_RasterizeFilter->SetDefaultBurnValue(0);
    m_RasterizeFilter->SetGlobalWarningDisplay(false);
    m_RasterizeFilter->SetBackgroundValue(intNoData);

    // Computing stats
    m_StatsFilter = StatsFilterType::New();
    m_StatsFilter->SetInput(img);
    m_StatsFilter->SetInputLabelImage(m_RasterizeFilter->GetOutput());
    m_StatsFilter->GetStreamer()->SetAutomaticAdaptativeStreaming(GetParameterInt("ram"));
    AddProcess(m_StatsFilter->GetStreamer(), "Computing statistics");
    m_StatsFilter->Update();

    // Remove the no-data entry
    StatsFilterType::LabelPopulationMapType countMap = m_StatsFilter->GetLabelPopulationMap();
    StatsFilterType::PixelValueMapType meanMap = m_StatsFilter->GetMeanValueMap();
    StatsFilterType::PixelValueMapType stdMap = m_StatsFilter->GetStandardDeviationValueMap();
    StatsFilterType::PixelValueMapType minMap = m_StatsFilter->GetMinValueMap();
    StatsFilterType::PixelValueMapType maxMap = m_StatsFilter->GetMaxValueMap();
    countMap.erase(intNoData);
    meanMap.erase(intNoData);
    stdMap.erase(intNoData);
    minMap.erase(intNoData);
    maxMap.erase(intNoData);

    // Add a statistics fields
    otbAppLogINFO("Writing output vector data");
    LabelValueType internalFID = 0;
    m_NewVectorData = VectorDataType::New();
    DataNodeType::Pointer root = m_NewVectorData->GetDataTree()->GetRoot()->Get();
    DataNodeType::Pointer document = DataNodeType::New();
    document->SetNodeType(otb::DOCUMENT);
    m_NewVectorData->GetDataTree()->Add(document, root);
    DataNodeType::Pointer folder = DataNodeType::New();
    folder->SetNodeType(otb::FOLDER);
    m_NewVectorData->GetDataTree()->Add(folder, document);
    m_NewVectorData->SetProjectionRef(m_VectorDataSrc->GetProjectionRef());
    TreeIteratorType itVector(m_VectorDataSrc->GetDataTree());
    itVector.GoToBegin();

    while (!itVector.IsAtEnd())
      {
      if (!itVector.Get()->IsRoot() && !itVector.Get()->IsDocument() && !itVector.Get()->IsFolder())
        {

        DataNodeType::Pointer currentGeometry = itVector.Get();

        // Add the geometry with the new fields
        if (countMap.count(internalFID) > 0)
          {
          currentGeometry->SetFieldAsDouble("count", countMap[internalFID] );
          for (unsigned int band = 0 ; band < img->GetNumberOfComponentsPerPixel() ; band++)
            {
            currentGeometry->SetFieldAsDouble(CreateFieldName("mean",  band), meanMap[internalFID][band] );
            currentGeometry->SetFieldAsDouble(CreateFieldName("stdev", band), stdMap [internalFID][band] );
            currentGeometry->SetFieldAsDouble(CreateFieldName("min",   band), minMap [internalFID][band] );
            currentGeometry->SetFieldAsDouble(CreateFieldName("max",   band), maxMap [internalFID][band] );
            }
          m_NewVectorData->GetDataTree()->Add(currentGeometry, folder);
          }

        internalFID++;
        }
      ++itVector;
      } // next feature

    SetParameterOutputVectorData("out", m_NewVectorData);

  }

  VectorDataType::Pointer m_VectorDataSrc;
  VectorDataType::Pointer m_NewVectorData;
  VectorDataReprojFilterType::Pointer m_VectorDataReprojectionFilter;
  RasterizeFilterType::Pointer m_RasterizeFilter;
  StatsFilterType::Pointer m_StatsFilter;

};
}
}

OTB_APPLICATION_EXPORT( otb::Wrapper::ZonalStatistics )
