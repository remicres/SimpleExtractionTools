/*=========================================================================

  Copyright (c) Remi Cresson (IRSTEA). All rights reserved.


     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#ifndef MODULES_REMOTE_SIMPLEEXTRACTIONTOOLS_INCLUDE_OTBCACHELESSLABELIMAGETOVECTORDATA_H_
#define MODULES_REMOTE_SIMPLEEXTRACTIONTOOLS_INCLUDE_OTBCACHELESSLABELIMAGETOVECTORDATA_H_

#include "otbImage.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkProcessObject.h"
#include "otbStreamingManager.h"
#include "otbLabelImageToVectorDataFilter.h"

namespace otb
{

/** \class CacheLessLabelImageToVectorData
 * \brief Produce a VectorData from an input pipeline. The pipeline is executed with
 * explicit streaming and the resulting image is stored in the internal cache of the filter.
 * This ensure that only the output image is cached, rather than all pipeline buffers.
 *
 * \ingroup SimpleExtractionTools
 */
template <class TInputImagePixel>
class ITK_EXPORT CacheLessLabelImageToVectorData :
    public VectorDataSource< otb::VectorData<double> >
{
public:
  /** Standard class typedefs. */
  typedef CacheLessLabelImageToVectorData                   Self;
  typedef VectorDataSource< VectorData<double> >            Superclass;
  typedef itk::SmartPointer<Self>                           Pointer;
  typedef itk::SmartPointer<const Self>                     ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(CacheLessLabelImageToVectorData, VectorDataSource);

  /** Some typedefs for the input. */
  typedef typename otb::Image<TInputImagePixel, 2>        InputImageType;
  typedef typename InputImageType::Pointer       InputImagePointer;
  typedef typename InputImageType::RegionType    InputImageRegionType;
  typedef typename InputImageType::PixelType     InputImagePixelType;
  typedef typename InputImageType::IndexType     InputIndexType;
  typedef itk::ImageRegionConstIterator<InputImageType> ConstIteratorType;
  typedef itk::ImageRegionIterator<InputImageType>      IteratorType;

  /** Definition of the output vector data. */
  typedef VectorData<double>                     VectorDataType;
  typedef typename VectorDataType::Pointer       VectorDataPointerType;

  typedef LabelImageToVectorDataFilter<InputImageType, double> LabelImageToVectorDataFilterType;

  /** Dimension of input image. */
  itkStaticConstMacro(InputImageDimension, unsigned int,
                      InputImageType::ImageDimension);

  /** Streaming manager base class pointer */
  typedef StreamingManager<InputImageType>       StreamingManagerType;
  typedef typename StreamingManagerType::Pointer StreamingManagerPointerType;

  /**  Return the StreamingManager object responsible for dividing
   *   the region to write */
  StreamingManagerType* GetStreamingManager(void)
    {
    return m_StreamingManager;
    }

  /**  Set a user-specified implementation of StreamingManager
   *   used to divide the largest possible region in several divisions */
  void SetStreamingManager(StreamingManagerType* streamingManager)
    {
    m_StreamingManager = streamingManager;
    }

  /**  Set the streaming mode to 'stripped' and configure the number of strips
   *   which will be used to stream the image */
  void SetNumberOfDivisionsStrippedStreaming(unsigned int nbDivisions);

  /**  Set the streaming mode to 'tiled' and configure the number of tiles
   *   which will be used to stream the image */
  void SetNumberOfDivisionsTiledStreaming(unsigned int nbDivisions);

  /**  Set the streaming mode to 'stripped' and configure the number of strips
   *   which will be used to stream the image with respect to a number of line
   *   per strip */
  void SetNumberOfLinesStrippedStreaming(unsigned int nbLinesPerStrip);

  /**  Set the streaming mode to 'stripped' and configure the number of MB
   *   available. The actual number of divisions is computed automatically
   *   by estimating the memory consumption of the pipeline.
   *   Setting the availableRAM parameter to 0 means that the available RAM
   *   is set from the CMake configuration option.
   *   The bias parameter is a multiplier applied on the estimated memory size
   *   of the pipeline and can be used to fine tune the potential gap between
   *   estimated memory and actual memory used, which can happen because of
   *   composite filters for example */
  void SetAutomaticStrippedStreaming(unsigned int availableRAM = 0, double bias = 1.0);

  /**  Set the streaming mode to 'tiled' and configure the dimension of the tiles
   *   in pixels for each dimension (square tiles will be generated) */
  void SetTileDimensionTiledStreaming(unsigned int tileDimension);

  /**  Set the streaming mode to 'tiled' and configure the number of MB
   *   available. The actual number of divisions is computed automatically
   *   by estimating the memory consumption of the pipeline.
   *   Tiles will be square.
   *   Setting the availableRAM parameter to 0 means that the available RAM
   *   is set from the CMake configuration option
   *   The bias parameter is a multiplier applied on the estimated memory size
   *   of the pipeline and can be used to fine tune the potential gap between
   *   estimated memory and actual memory used, which can happen because of
   *   composite filters for example */
  void SetAutomaticTiledStreaming(unsigned int availableRAM = 0, double bias = 1.0);

  /**  Set the streaming mode to 'adaptative' and configure the number of MB
   *   available. The actual number of divisions is computed automatically
   *   by estimating the memory consumption of the pipeline.
   *   Tiles will try to match the input file tile scheme.
   *   Setting the availableRAM parameter to 0 means that the available RAM
   *   is set from the CMake configuration option */
  void SetAutomaticAdaptativeStreaming(unsigned int availableRAM = 0, double bias = 1.0);

  /** Set the only input of the writer */
  using Superclass::SetInput;
  virtual void SetInput(const InputImageType *input);

  /** Get writer only input */
  const InputImageType* GetInput();

  /** Override Update() from ProcessObject because this filter
   *  has no output. */
  void Update() ITK_OVERRIDE;

  void GenerateInputRequestedRegion() {};

  std::string GetFieldName() { return vectorizeFilter->GetFieldName(); }

protected:
  CacheLessLabelImageToVectorData();
  ~CacheLessLabelImageToVectorData() ITK_OVERRIDE;

private:
  CacheLessLabelImageToVectorData(const CacheLessLabelImageToVectorData &); //purposely not implemented
  void operator =(const CacheLessLabelImageToVectorData&); //purposely not implemented

  void ObserveSourceFilterProgress(itk::Object* object, const itk::EventObject & event )
  {
    if (typeid(event) != typeid(itk::ProgressEvent))
      {
      return;
      }

    itk::ProcessObject* processObject = dynamic_cast<itk::ProcessObject*>(object);
    if (processObject)
      {
      m_DivisionProgress = processObject->GetProgress();
      }

    this->UpdateFilterProgress();
  }

  void UpdateFilterProgress()
  {
    this->UpdateProgress( (m_DivisionProgress + m_CurrentDivision) / m_NumberOfDivisions );
  }

  unsigned int m_NumberOfDivisions;
  unsigned int m_CurrentDivision;
  float m_DivisionProgress;

  StreamingManagerPointerType m_StreamingManager;

  bool          m_IsObserving;
  unsigned long m_ObserverID;

  typename InputImageType::Pointer  bufferedInputImage;
  typename LabelImageToVectorDataFilterType::Pointer vectorizeFilter;
};

} // end namespace otb

#ifndef OTB_MANUAL_INSTANTIATION
#include "otbCacheLessLabelImageToVectorData.txx"
#endif

#endif /* MODULES_REMOTE_SIMPLEEXTRACTIONTOOLS_INCLUDE_OTBCACHELESSLABELIMAGETOVECTORDATA_H_ */
