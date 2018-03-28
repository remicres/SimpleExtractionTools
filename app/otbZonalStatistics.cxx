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
#include "otbVectorDataToLabelImageFilter.h"
#include "otbVectorDataIntoImageProjectionFilter.h"

// Vnl vector
#include "vnl/vnl_vector.h"

// itk iterator
#include "itkImageRegionConstIterator.h"

using namespace std;

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
  typedef size_t                                                      LabelValueType;
  typedef otb::Image<LabelValueType, 2>                               LabelImageType;
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
  typedef vnl_matrix<double>                                          RealMatrix;
  typedef vnl_vector<LabelValueType>                                  SizeVector;

  /** Image iterator */
  typedef itk::ImageRegionConstIterator<LabelImageType>               LabelIteratorType;
  typedef itk::ImageRegionConstIterator<FloatVectorImageType>         ImageIteratorType;

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
      vdSrc = m_VectorDataReprojectionFilter->GetOutput();
      }
    else
      {
      vdSrc = shp;
      }

    // Add a FID field
    LabelValueType internalFID = 1;
    const string internalFIDField = "__fid___";
    vdWithFid = VectorDataType::New();
    DataNodeType::Pointer root1 = vdWithFid->GetDataTree()->GetRoot()->Get();
    DataNodeType::Pointer document1 = DataNodeType::New();
    document1->SetNodeType(otb::DOCUMENT);
    vdWithFid->GetDataTree()->Add(document1, root1);
    DataNodeType::Pointer folder1 = DataNodeType::New();
    folder1->SetNodeType(otb::FOLDER);
    vdWithFid->GetDataTree()->Add(folder1, document1);
    vdWithFid->SetProjectionRef(vdSrc->GetProjectionRef());
    TreeIteratorType itVector1(vdSrc->GetDataTree());
    itVector1.GoToBegin();
    while (!itVector1.IsAtEnd())
      {
      if (!itVector1.Get()->IsRoot() && !itVector1.Get()->IsDocument() && !itVector1.Get()->IsFolder())
        {
        DataNodeType::Pointer currentGeometry = itVector1.Get();
        currentGeometry->SetFieldAsInt(internalFIDField, internalFID );
        vdWithFid->GetDataTree()->Add(currentGeometry, folder1);
        internalFID++;
        }
      ++itVector1;
      } // next feature

    // Rasterize vector data
    m_RasterizeFilter = RasterizeFilterType::New();
    m_RasterizeFilter->AddVectorData(vdWithFid);
    m_RasterizeFilter->SetOutputOrigin(img->GetOrigin());
    m_RasterizeFilter->SetOutputSpacing(img->GetSignedSpacing());
    m_RasterizeFilter->SetOutputSize(img->GetLargestPossibleRegion().GetSize());
    m_RasterizeFilter->SetOutputProjectionRef(img->GetProjectionRef());
    m_RasterizeFilter->SetBurnAttribute(internalFIDField);
    m_RasterizeFilter->UpdateOutputInformation();

    // Computing stats
    otbAppLogINFO("Computing stats");

    // Explicit streaming over the input target image, based on the RAM parameter
    typedef otb::RAMDrivenStrippedStreamingManager<FloatVectorImageType> StreamingManagerType;
    StreamingManagerType::Pointer m_StreamingManager = StreamingManagerType::New();
    m_StreamingManager->SetAvailableRAMInMB(GetParameterInt("ram"));
    m_StreamingManager->PrepareStreaming(img, img->GetLargestPossibleRegion() );

    // Init. stats containers
    const LabelValueType N = internalFID;
    const unsigned int nBands = img->GetNumberOfComponentsPerPixel();
    RealMatrix minimums(nBands, N, itk::NumericTraits<double>::max());
    RealMatrix maximums(nBands, N, itk::NumericTraits<double>::min());
    RealMatrix sum     (nBands, N, 0);
    RealMatrix sqSum   (nBands, N, 0);
    RealMatrix means   (nBands, N, 0);
    RealMatrix stdevs  (nBands, N, 0);
    SizeVector counts(N,0);
    if (m_RasterizeFilter->GetOutput()->GetLargestPossibleRegion().GetNumberOfPixels() != img->GetLargestPossibleRegion().GetNumberOfPixels())
      {
      otbAppLogFATAL("Rasterized image and input image don't have the same number of pixels");
      }

    int m_NumberOfDivisions = m_StreamingManager->GetNumberOfSplits();
    for (int m_CurrentDivision = 0; m_CurrentDivision < m_NumberOfDivisions; m_CurrentDivision++)
      {

      FloatVectorImageType::RegionType streamRegion = m_StreamingManager->GetSplit(m_CurrentDivision);

      img->SetRequestedRegion(streamRegion);
      img->PropagateRequestedRegion();
      img->UpdateOutputData();

      m_RasterizeFilter->GetOutput()->SetRequestedRegion(streamRegion);
      m_RasterizeFilter->GetOutput()->PropagateRequestedRegion();
      m_RasterizeFilter->GetOutput()->UpdateOutputData();

      LabelIteratorType it(m_RasterizeFilter->GetOutput(), streamRegion);
      ImageIteratorType it_in(img, streamRegion);
      for (it.GoToBegin(), it_in.GoToBegin(); !it.IsAtEnd(); ++it, ++it_in)
        {
        const LabelValueType fid = it.Get();
        const FloatVectorImageType::PixelType pix = it_in.Get();
        bool valid = true;
        if (has_nodata)
          {
          for (unsigned int band = 0 ; band < nBands ; band++)
            {
            const FloatVectorImageType::InternalPixelType val = pix[band];
            if (val == m_InputNoData)
              {
              valid = false;
              break;
              }
            }
          }
        if (valid)
          {
          for (unsigned int band = 0 ; band < nBands ; band++)
            {
            const FloatVectorImageType::InternalPixelType val = pix[band];
            if (val < minimums[band][fid])
              minimums[band][fid] = val;
            if (val > maximums[band][fid])
              maximums[band][fid] = val;
            sum[band][fid]+=val;
            sqSum[band][fid]+=val*val;
            } // next band
          counts[fid]++;
          }
        } // next pixel
      }

    // Summarize stats
    otbAppLogINFO("Summarizing stats");
    for (LabelValueType fid = 0 ; fid < N ; fid++)
      {
      const LabelValueType count = counts[fid];
      for (unsigned int band = 0 ; band < nBands ; band++)
        {
        if (count>0)
          {
          means[band][fid] = sum[band][fid] / static_cast<FloatVectorImageType::InternalPixelType>(count);

          // Unbiased estimate
          FloatVectorImageType::InternalPixelType variance = 0;
          if (count > 1)
            {
            variance =
                (sqSum[band][fid] - (sum[band][fid]*sum[band][fid] / static_cast<FloatVectorImageType::InternalPixelType>(count) ) ) /
                (static_cast<FloatVectorImageType::InternalPixelType>(count) - 1);
            if (variance > 0)
              {
              stdevs[band][fid] = vcl_sqrt(variance);
              }
            }
          }
        }
      }

    // Add a statistics fields
    otbAppLogINFO("Writing output vector data");
    internalFID = 1;
    vdWithStats = VectorDataType::New();
    DataNodeType::Pointer root = vdWithStats->GetDataTree()->GetRoot()->Get();
    DataNodeType::Pointer document = DataNodeType::New();
    document->SetNodeType(otb::DOCUMENT);
    vdWithStats->GetDataTree()->Add(document, root);
    DataNodeType::Pointer folder = DataNodeType::New();
    folder->SetNodeType(otb::FOLDER);
    vdWithStats->GetDataTree()->Add(folder, document);
    vdWithStats->SetProjectionRef(vdSrc->GetProjectionRef());
    TreeIteratorType itVector(vdSrc->GetDataTree());
    itVector.GoToBegin();
    while (!itVector.IsAtEnd())
      {
      if (!itVector.Get()->IsRoot() && !itVector.Get()->IsDocument() && !itVector.Get()->IsFolder())
        {
        DataNodeType::Pointer currentGeometry = itVector.Get();
        for (unsigned int band = 0 ; band < nBands ; band++)
          {
          currentGeometry->SetFieldAsDouble(CreateFieldName("mean",  band), means   [band][internalFID] );
          currentGeometry->SetFieldAsDouble(CreateFieldName("stdev", band), stdevs  [band][internalFID] );
          currentGeometry->SetFieldAsDouble(CreateFieldName("min",   band), minimums[band][internalFID] );
          currentGeometry->SetFieldAsDouble(CreateFieldName("max",   band), maximums[band][internalFID] );
          }
        currentGeometry->SetFieldAsDouble("count", static_cast<double>(counts[internalFID]) );

        vdWithStats->GetDataTree()->Add(currentGeometry, folder);
        internalFID++;
        }
      ++itVector;
      } // next feature

    SetParameterOutputVectorData("out", vdWithStats);

  }

  VectorDataType::Pointer vdSrc;
  VectorDataType::Pointer vdWithFid;
  VectorDataType::Pointer vdWithStats;
  VectorDataReprojFilterType::Pointer m_VectorDataReprojectionFilter;
  RasterizeFilterType::Pointer m_RasterizeFilter;

};
}
}

OTB_APPLICATION_EXPORT( otb::Wrapper::ZonalStatistics )
