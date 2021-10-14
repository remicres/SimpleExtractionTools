/*=========================================================================

  Copyright (c) Remi Cresson (IRSTEA). All rights reserved.


     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __MeanResampleImageFilter_hxx
#define __MeanResampleImageFilter_hxx

#include <otbMeanResampleImageFilter.h>
#include "itkProgressReporter.h"

namespace otb
{
/**
 *
 */
template <class TImage>
MeanResampleImageFilter<TImage>
::MeanResampleImageFilter()
 {
  m_StepX = 1;
  m_StepY = 1;
  m_NoDataValue = 0;
 }

template <class TImage>
void
MeanResampleImageFilter<TImage>
::GenerateOutputInformation()
 {
  Superclass::GenerateOutputInformation();

  // Grab input image
  ImageType * inputImage = static_cast<ImageType * >(
      Superclass::ProcessObject::GetInput(0) );

  ImageType * outputPtr = this->GetOutput();

  // The new output image has the same origin
  ImagePointType origin = inputImage->GetOrigin();
  origin[0] += 0.5 * inputImage->GetSpacing()[0] * (m_StepX - 1);
  origin[1] += 0.5 * inputImage->GetSpacing()[1] * (m_StepY - 1);
  outputPtr->SetOrigin ( origin );

  // New spacing for the output image
  ImageSpacingType spacing = inputImage->GetSpacing();
  spacing[0] *= m_StepX;
  spacing[1] *= m_StepY;
  outputPtr->SetSpacing (spacing);

  // New size for the output image
  ImageRegionType inRegion = inputImage->GetLargestPossibleRegion();
  ImageRegionType outRegion;
  outRegion.SetIndex(0, 0);
  outRegion.SetIndex(1, 0);
  outRegion.SetSize (0, inRegion.GetSize()[0] / m_StepX);
  outRegion.SetSize (1, inRegion.GetSize()[1] / m_StepY);
  outputPtr->SetLargestPossibleRegion( outRegion );

 }

template <class TImage>
void
MeanResampleImageFilter<TImage>
::GenerateInputRequestedRegion()
 {

  // Output requested region
  const ImageRegionType outRegion = this->GetOutput()->GetRequestedRegion();

  // Grab input image
  ImageType * inputImage = static_cast<ImageType * >(
      Superclass::ProcessObject::GetInput(0) );
  ImageRegionType inRegion;
  inRegion.SetIndex(0, outRegion.GetIndex()[0] * m_StepX);
  inRegion.SetIndex(1, outRegion.GetIndex()[1] * m_StepY);
  inRegion.SetSize (0, outRegion.GetSize()[0]  * m_StepX);
  inRegion.SetSize (1, outRegion.GetSize()[1]  * m_StepY);
  inRegion.Crop(inputImage->GetLargestPossibleRegion());
  inputImage->SetRequestedRegion(inRegion);
 }

/**
 *
 */
template <class TImage>
void
MeanResampleImageFilter<TImage>
::ThreadedGenerateData(const ImageRegionType& outputRegionForThread, itk::ThreadIdType threadId)
 {

  // Support progress methods/callbacks
  itk::ProgressReporter progress(this, threadId, outputRegionForThread.GetNumberOfPixels() );

  // Iterate through the thread region
  OutputImageIteratorType outputIt(this->GetOutput(), outputRegionForThread);

  // Grab input image
  ImageType * inputImage = static_cast<ImageType * >(
      Superclass::ProcessObject::GetInput(0) );

  for ( outputIt.GoToBegin(); !outputIt.IsAtEnd(); ++outputIt)
    {

    // sum
    float accum = 0.0;
    float npix = 0.0;

    for (unsigned int x = 0; x < m_StepX ; x++)
      for (unsigned int y = 0; y < m_StepY ; y++)
        {
        ImageIndexType index = outputIt.GetIndex();
        index[0] *= m_StepX;
        index[1] *= m_StepY;
        index[0] += x;
        index[1] += y;
        if (inputImage->GetLargestPossibleRegion().IsInside(index))
          {
          float pixVal = inputImage->GetPixel(index);
          if (pixVal != m_NoDataValue)
            {
            accum += pixVal;
            npix += 1.0;
            }
          }
        }


    // normalize
    if (npix > 0.0)
      accum /= npix;

    outputIt.Set(accum);

    progress.CompletedPixel();
    } // Next pixel
 }
}
#endif



