/*=========================================================================

  Copyright (c) Remi Cresson (IRSTEA). All rights reserved.


     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#ifndef MODULES_REMOTE_SIMPLEEXTRACTIONTOOLS_INCLUDE_OTBCACHELESSLABELIMAGETOVECTORDATA_txx_
#define MODULES_REMOTE_SIMPLEEXTRACTIONTOOLS_INCLUDE_OTBCACHELESSLABELIMAGETOVECTORDATA_txx_

#include "otbCacheLessLabelImageToVectorData.h"

#include "itkObjectFactoryBase.h"

#include "itkImageRegionMultidimensionalSplitter.h"

#include "itkImageRegionIterator.h"

#include "otbNumberOfDivisionsStrippedStreamingManager.h"
#include "otbNumberOfDivisionsTiledStreamingManager.h"
#include "otbNumberOfLinesStrippedStreamingManager.h"
#include "otbRAMDrivenStrippedStreamingManager.h"
#include "otbTileDimensionTiledStreamingManager.h"
#include "otbRAMDrivenTiledStreamingManager.h"
#include "otbRAMDrivenAdaptativeStreamingManager.h"

namespace otb
{

/**
 *
 */
template <class TInputImagePixel>
CacheLessLabelImageToVectorData<TInputImagePixel>
::CacheLessLabelImageToVectorData()
 : m_NumberOfDivisions(0),
   m_CurrentDivision(0),
   m_DivisionProgress(0.0),
   m_IsObserving(true),
   m_ObserverID(0)
   {

  // By default, we use tiled streaming, with automatic tile size
  // We don't set any parameter, so the memory size is retrieved from the OTB configuration options
  this->SetAutomaticAdaptativeStreaming();
   }

/**
 *
 */
template <class TInputImagePixel>
CacheLessLabelImageToVectorData<TInputImagePixel>
::~CacheLessLabelImageToVectorData()
{
}

template <class TInputImagePixel>
void
CacheLessLabelImageToVectorData<TInputImagePixel>
::SetNumberOfDivisionsStrippedStreaming(unsigned int nbDivisions)
 {
  typedef NumberOfDivisionsStrippedStreamingManager<InputImageType> NumberOfDivisionsStrippedStreamingManagerType;
  typename NumberOfDivisionsStrippedStreamingManagerType::Pointer streamingManager = NumberOfDivisionsStrippedStreamingManagerType::New();
  streamingManager->SetNumberOfDivisions(nbDivisions);

  m_StreamingManager = streamingManager;
 }

template <class TInputImagePixel>
void
CacheLessLabelImageToVectorData<TInputImagePixel>
::SetNumberOfDivisionsTiledStreaming(unsigned int nbDivisions)
 {
  typedef NumberOfDivisionsTiledStreamingManager<InputImageType> NumberOfDivisionsTiledStreamingManagerType;
  typename NumberOfDivisionsTiledStreamingManagerType::Pointer streamingManager = NumberOfDivisionsTiledStreamingManagerType::New();
  streamingManager->SetNumberOfDivisions(nbDivisions);

  m_StreamingManager = streamingManager;
 }

template <class TInputImagePixel>
void
CacheLessLabelImageToVectorData<TInputImagePixel>
::SetNumberOfLinesStrippedStreaming(unsigned int nbLinesPerStrip)
 {
  typedef NumberOfLinesStrippedStreamingManager<InputImageType> NumberOfLinesStrippedStreamingManagerType;
  typename NumberOfLinesStrippedStreamingManagerType::Pointer streamingManager = NumberOfLinesStrippedStreamingManagerType::New();
  streamingManager->SetNumberOfLinesPerStrip(nbLinesPerStrip);

  m_StreamingManager = streamingManager;
 }

template <class TInputImagePixel>
void
CacheLessLabelImageToVectorData<TInputImagePixel>
::SetAutomaticStrippedStreaming(unsigned int availableRAM, double bias)
 {
  typedef RAMDrivenStrippedStreamingManager<InputImageType> RAMDrivenStrippedStreamingManagerType;
  typename RAMDrivenStrippedStreamingManagerType::Pointer streamingManager = RAMDrivenStrippedStreamingManagerType::New();
  streamingManager->SetAvailableRAMInMB(availableRAM);
  streamingManager->SetBias(bias);

  m_StreamingManager = streamingManager;
 }

template <class TInputImagePixel>
void
CacheLessLabelImageToVectorData<TInputImagePixel>
::SetTileDimensionTiledStreaming(unsigned int tileDimension)
 {
  typedef TileDimensionTiledStreamingManager<InputImageType> TileDimensionTiledStreamingManagerType;
  typename TileDimensionTiledStreamingManagerType::Pointer streamingManager = TileDimensionTiledStreamingManagerType::New();
  streamingManager->SetTileDimension(tileDimension);

  m_StreamingManager = streamingManager;
 }

template <class TInputImagePixel>
void
CacheLessLabelImageToVectorData<TInputImagePixel>
::SetAutomaticTiledStreaming(unsigned int availableRAM, double bias)
 {
  typedef RAMDrivenTiledStreamingManager<InputImageType> RAMDrivenTiledStreamingManagerType;
  typename RAMDrivenTiledStreamingManagerType::Pointer streamingManager = RAMDrivenTiledStreamingManagerType::New();
  streamingManager->SetAvailableRAMInMB(availableRAM);
  streamingManager->SetBias(bias);
  m_StreamingManager = streamingManager;
 }

template <class TInputImagePixel>
void
CacheLessLabelImageToVectorData<TInputImagePixel>
::SetAutomaticAdaptativeStreaming(unsigned int availableRAM, double bias)
 {
  typedef RAMDrivenAdaptativeStreamingManager<InputImageType> RAMDrivenAdaptativeStreamingManagerType;
  typename RAMDrivenAdaptativeStreamingManagerType::Pointer streamingManager = RAMDrivenAdaptativeStreamingManagerType::New();
  streamingManager->SetAvailableRAMInMB(availableRAM);
  streamingManager->SetBias(bias);
  m_StreamingManager = streamingManager;
 }

template<class TInputImagePixel>
void
CacheLessLabelImageToVectorData<TInputImagePixel>
::SetInput(const InputImageType* input)
 {
  this->ProcessObject::SetNthInput(0,const_cast<InputImageType*>(input));
 }

template<class TInputImagePixel>
const typename CacheLessLabelImageToVectorData<TInputImagePixel>::InputImageType*
CacheLessLabelImageToVectorData<TInputImagePixel>
::GetInput()
 {
  if (this->GetNumberOfInputs() < 1)
    {
      return ITK_NULLPTR;
    }

  return static_cast<const InputImageType*>(this->ProcessObject::GetInput(0));
 }

/**
 * Update method : update output information of input and write to file
 */
template<class TInputImagePixel>
void
CacheLessLabelImageToVectorData<TInputImagePixel>
::Update()
 {
  // Update output information on input image
  InputImagePointer inputPtr =
      const_cast<InputImageType *>(this->GetInput());

  // Make sure input is available
  if ( inputPtr.IsNull() )
    {
      itkExceptionMacro(<< "No input to writer");
    }

  this->SetAbortGenerateData(0);
  this->SetProgress(0.0);

  /**
   * Tell all Observers that the filter is starting
   */
  this->InvokeEvent(itk::StartEvent());

  /**
   * Grab the input
   */
  inputPtr->UpdateOutputInformation();
  InputImageRegionType inputRegion = inputPtr->GetLargestPossibleRegion();

  // Allocate the buffer image
  bufferedInputImage = InputImageType::New();
  bufferedInputImage->SetRegions(inputRegion);
  bufferedInputImage->Allocate();
  bufferedInputImage->SetMetaDataDictionary(inputPtr->GetMetaDataDictionary());
  bufferedInputImage->SetSpacing(inputPtr->GetSignedSpacing());
  bufferedInputImage->SetOrigin (inputPtr->GetOrigin() );


  /** Compare the buffered region  with the inputRegion which is the largest
   * possible region or a user defined region through extended filename
   * Not sure that if this modification is needed  */
  if (inputPtr->GetBufferedRegion() == inputRegion)
    {
      otbMsgDevMacro(<< "Buffered region is the largest possible region, there is no need for streaming.");
      this->SetNumberOfDivisionsStrippedStreaming(1);
    }
  m_StreamingManager->PrepareStreaming(inputPtr, inputRegion);
  m_NumberOfDivisions = m_StreamingManager->GetNumberOfSplits();
  otbMsgDebugMacro(<< "Number Of Stream Divisions : " << m_NumberOfDivisions);

  /**
   * Loop over the number of pieces, execute the upstream pipeline on each
   * piece, and copy the results into the output image.
   */
  InputImageRegionType streamRegion;

  this->UpdateProgress(0);
  m_CurrentDivision = 0;
  m_DivisionProgress = 0;

  // Get the source process object
  itk::ProcessObject* source = inputPtr->GetSource();
  m_IsObserving = false;
  m_ObserverID = 0;

  // Check if source exists
  if(source)
    {
      typedef itk::MemberCommand<Self>      CommandType;
      typedef typename CommandType::Pointer CommandPointerType;

      CommandPointerType command = CommandType::New();
      command->SetCallbackFunction(this, &Self::ObserveSourceFilterProgress);

      m_ObserverID = source->AddObserver(itk::ProgressEvent(), command);
      m_IsObserving = true;
    }
  else
    {
      itkWarningMacro(<< "Could not get the source process object. Progress report might be buggy");
    }

  for (m_CurrentDivision = 0;
      m_CurrentDivision < m_NumberOfDivisions && !this->GetAbortGenerateData();
      m_CurrentDivision++, m_DivisionProgress = 0, this->UpdateFilterProgress())
    {
      streamRegion = m_StreamingManager->GetSplit(m_CurrentDivision);

      inputPtr->SetRequestedRegion(streamRegion);
      inputPtr->PropagateRequestedRegion();
      inputPtr->UpdateOutputData();

      // Copy output in buffer
      ConstIteratorType inIt(inputPtr, streamRegion);
      IteratorType outIt(bufferedInputImage, streamRegion);
      for (inIt.GoToBegin(), outIt.GoToBegin(); !inIt.IsAtEnd(); ++outIt, ++inIt)
        {
          outIt.Set(inIt.Get());
        }

    }

  // Vectorize the buffered image
  vectorizeFilter = LabelImageToVectorDataFilterType::New();
  vectorizeFilter->SetInput(bufferedInputImage);
  vectorizeFilter->SetInputMask(bufferedInputImage);
  vectorizeFilter->Update();
  this->GraftOutput( vectorizeFilter->GetOutput() );

  /**
   * If we ended due to aborting, push the progress up to 1.0 (since
   * it probably didn't end there)
   */
  if (!this->GetAbortGenerateData())
    {
      this->UpdateProgress(1.0);
    }

  // Notify end event observers
  this->InvokeEvent(itk::EndEvent());

  if (m_IsObserving)
    {
      m_IsObserving = false;
      source->RemoveObserver(m_ObserverID);
    }

  /**
   * Release any inputs if marked for release
   */
  this->ReleaseInputs();

 }


} // end namespace otb

#endif
