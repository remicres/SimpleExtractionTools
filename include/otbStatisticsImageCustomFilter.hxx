/*=========================================================================

  Copyright (c) Remi Cresson (IRSTEA). All rights reserved.


     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __otbStatisticsImageCustomFilter_hxx
#define __otbStatisticsImageCustomFilter_hxx
#include "otbStatisticsImageCustomFilter.h"


#include "itkImageScanlineIterator.h"
#include "itkProgressReporter.h"

namespace otb
{
template< typename TInputImage >
StatisticsImageCustomFilter< TInputImage >
::StatisticsImageCustomFilter():m_ThreadSum(1), m_SumOfSquares(1),
 m_Count(1), m_ThreadMin(1), m_ThreadMax(1)
{
  // first output is a copy of the image, DataObject created by
  // superclass
  //
  // allocate the data objects for the outputs which are
  // just decorators around pixel types
  for ( int i = 1; i < 3; ++i )
    {
    typename PixelObjectType::Pointer output =
      static_cast< PixelObjectType * >( this->MakeOutput(i).GetPointer() );
    this->itk::ProcessObject::SetNthOutput( i, output.GetPointer() );
    }
  // allocate the data objects for the outputs which are
  // just decorators around real types
  for ( int i = 3; i < 7; ++i )
    {
    typename RealObjectType::Pointer output =
      static_cast< RealObjectType * >( this->MakeOutput(i).GetPointer() );
    this->itk::ProcessObject::SetNthOutput( i, output.GetPointer() );
    }

  // allocate the data objects for the count output (long type)
  typename LongObjectType::Pointer output =
        static_cast< LongObjectType * >( this->MakeOutput(7).GetPointer() );
  this->itk::ProcessObject::SetNthOutput( 7, output.GetPointer() );

  this->GetMinimumOutput()->Set( itk::NumericTraits< PixelType >::max() );
  this->GetMaximumOutput()->Set( itk::NumericTraits< PixelType >::NonpositiveMin() );
  this->GetMeanOutput()->Set( itk::NumericTraits< RealType >::max() );
  this->GetSigmaOutput()->Set( itk::NumericTraits< RealType >::max() );
  this->GetVarianceOutput()->Set( itk::NumericTraits< RealType >::max() );
  this->GetSumOutput()->Set(itk::NumericTraits< RealType >::Zero);
  this->GetCountOutput()->Set(itk::NumericTraits< LongType >::Zero);

  m_IgnoredInputValue = itk::NumericTraits<PixelType>::max();
}

template< typename TInputImage >
itk::DataObject::Pointer
StatisticsImageCustomFilter< TInputImage >
::MakeOutput(DataObjectPointerArraySizeType output)
{
  switch ( output )
    {
    case 0:
      return TInputImage::New().GetPointer();
      break;
    case 1:
      return PixelObjectType::New().GetPointer();
      break;
    case 2:
      return PixelObjectType::New().GetPointer();
      break;
    case 3:
    case 4:
    case 5:
    case 6:
      return RealObjectType::New().GetPointer();
      break;
    case 7:
      return LongObjectType::New().GetPointer();
      break;
    default:
      // might as well make an image
      return TInputImage::New().GetPointer();
      break;
    }
}

template< typename TInputImage >
typename StatisticsImageCustomFilter< TInputImage >::PixelObjectType *
StatisticsImageCustomFilter< TInputImage >
::GetMinimumOutput()
{
  return static_cast< PixelObjectType * >( this->itk::ProcessObject::GetOutput(1) );
}

template< typename TInputImage >
const typename StatisticsImageCustomFilter< TInputImage >::PixelObjectType *
StatisticsImageCustomFilter< TInputImage >
::GetMinimumOutput() const
{
  return static_cast< const PixelObjectType * >( this->itk::ProcessObject::GetOutput(1) );
}

template< typename TInputImage >
typename StatisticsImageCustomFilter< TInputImage >::PixelObjectType *
StatisticsImageCustomFilter< TInputImage >
::GetMaximumOutput()
{
  return static_cast< PixelObjectType * >( this->itk::ProcessObject::GetOutput(2) );
}

template< typename TInputImage >
const typename StatisticsImageCustomFilter< TInputImage >::PixelObjectType *
StatisticsImageCustomFilter< TInputImage >
::GetMaximumOutput() const
{
  return static_cast< const PixelObjectType * >( this->itk::ProcessObject::GetOutput(2) );
}

template< typename TInputImage >
typename StatisticsImageCustomFilter< TInputImage >::RealObjectType *
StatisticsImageCustomFilter< TInputImage >
::GetMeanOutput()
{
  return static_cast< RealObjectType * >( this->itk::ProcessObject::GetOutput(3) );
}

template< typename TInputImage >
const typename StatisticsImageCustomFilter< TInputImage >::RealObjectType *
StatisticsImageCustomFilter< TInputImage >
::GetMeanOutput() const
{
  return static_cast< const RealObjectType * >( this->itk::ProcessObject::GetOutput(3) );
}

template< typename TInputImage >
typename StatisticsImageCustomFilter< TInputImage >::RealObjectType *
StatisticsImageCustomFilter< TInputImage >
::GetSigmaOutput()
{
  return static_cast< RealObjectType * >( this->itk::ProcessObject::GetOutput(4) );
}

template< typename TInputImage >
const typename StatisticsImageCustomFilter< TInputImage >::RealObjectType *
StatisticsImageCustomFilter< TInputImage >
::GetSigmaOutput() const
{
  return static_cast< const RealObjectType * >( this->itk::ProcessObject::GetOutput(4) );
}

template< typename TInputImage >
typename StatisticsImageCustomFilter< TInputImage >::RealObjectType *
StatisticsImageCustomFilter< TInputImage >
::GetVarianceOutput()
{
  return static_cast< RealObjectType * >( this->itk::ProcessObject::GetOutput(5) );
}

template< typename TInputImage >
const typename StatisticsImageCustomFilter< TInputImage >::RealObjectType *
StatisticsImageCustomFilter< TInputImage >
::GetVarianceOutput() const
{
  return static_cast< const RealObjectType * >( this->itk::ProcessObject::GetOutput(5) );
}

template< typename TInputImage >
typename StatisticsImageCustomFilter< TInputImage >::RealObjectType *
StatisticsImageCustomFilter< TInputImage >
::GetSumOutput()
{
  return static_cast< RealObjectType * >( this->itk::ProcessObject::GetOutput(6) );
}

template< typename TInputImage >
const typename StatisticsImageCustomFilter< TInputImage >::RealObjectType *
StatisticsImageCustomFilter< TInputImage >
::GetSumOutput() const
{
  return static_cast< const RealObjectType * >( this->itk::ProcessObject::GetOutput(6) );
}

template< typename TInputImage >
typename StatisticsImageCustomFilter< TInputImage >::RealObjectType *
StatisticsImageCustomFilter< TInputImage >
::GetSumOfSquaresOutput()
{
  return static_cast< RealObjectType * >( this->itk::ProcessObject::GetOutput(6) );
}

template< typename TInputImage >
const typename StatisticsImageCustomFilter< TInputImage >::RealObjectType *
StatisticsImageCustomFilter< TInputImage >
::GetSumOfSquaresOutput() const
{
  return static_cast< const RealObjectType * >( this->itk::ProcessObject::GetOutput(6) );
}

template< typename TInputImage >
typename StatisticsImageCustomFilter< TInputImage >::LongObjectType *
StatisticsImageCustomFilter< TInputImage >
::GetCountOutput()
{
  return static_cast< LongObjectType * >( this->itk::ProcessObject::GetOutput(7) );
}

template< typename TInputImage >
const typename StatisticsImageCustomFilter< TInputImage >::LongObjectType *
StatisticsImageCustomFilter< TInputImage >
::GetCountOutput() const
{
  return static_cast< const LongObjectType * >( this->itk::ProcessObject::GetOutput(7) );
}

template< typename TInputImage >
void
StatisticsImageCustomFilter< TInputImage >
::AllocateOutputs()
{
  // Pass the input through as the output
  InputImagePointer image =
    const_cast< TInputImage * >( this->GetInput() );

  this->GraftOutput(image);

  // Nothing that needs to be allocated for the remaining outputs
}

template< typename TInputImage >
void
StatisticsImageCustomFilter< TInputImage >
::BeforeThreadedGenerateData()
{
  itk::ThreadIdType numberOfThreads = this->GetNumberOfThreads();

  // Resize the thread temporaries
  m_Count.SetSize(numberOfThreads);
  m_SumOfSquares.SetSize(numberOfThreads);
  m_ThreadSum.SetSize(numberOfThreads);
  m_ThreadMin.SetSize(numberOfThreads);
  m_ThreadMax.SetSize(numberOfThreads);

  // Initialize the temporaries
  m_Count.Fill(itk::NumericTraits< itk::SizeValueType >::Zero);
  m_ThreadSum.Fill(itk::NumericTraits< RealType >::Zero);
  m_SumOfSquares.Fill(itk::NumericTraits< RealType >::Zero);
  m_ThreadMin.Fill( itk::NumericTraits< PixelType >::max() );
  m_ThreadMax.Fill( itk::NumericTraits< PixelType >::NonpositiveMin() );
}

template< typename TInputImage >
void
StatisticsImageCustomFilter< TInputImage >
::AfterThreadedGenerateData()
{
  itk::ThreadIdType    i;

  itk::ThreadIdType numberOfThreads = this->GetNumberOfThreads();

  PixelType minimum = this->GetMinimumOutput()->Get();
  PixelType maximum = this->GetMaximumOutput()->Get();
  RealType  sum = this ->GetSumOutput()->Get();
  RealType  sumOfSquares = this->GetSumOfSquaresOutput()->Get();
  LongType  count = this->GetCountOutput()->Get();

  RealType  mean, sigma, variance;
  mean = sigma = variance = itk::NumericTraits< RealType >::ZeroValue();

  // Find the min/max over all threads and accumulate count, sum and
  // sum of squares
  for ( i = 0; i < numberOfThreads; i++ )
    {
    count += m_Count[i];
    sum += m_ThreadSum[i];
    sumOfSquares += m_SumOfSquares[i];

    if ( m_ThreadMin[i] < minimum )
      {
      minimum = m_ThreadMin[i];
      }
    if ( m_ThreadMax[i] > maximum )
      {
      maximum = m_ThreadMax[i];
      }
    }

  // compute statistics
  if (count > 1)
    {
    mean = sum / static_cast< RealType >( count );

    // unbiased estimate
    variance = ( sumOfSquares - ( sum * sum / static_cast< RealType >( count ) ) )
                 / ( static_cast< RealType >( count ) - 1 );
    sigma = std::sqrt(variance);
    }

  // Update the outputs
  this->GetMinimumOutput()->Set(minimum);
  this->GetMaximumOutput()->Set(maximum);
  this->GetMeanOutput()->Set(mean);
  this->GetSigmaOutput()->Set(sigma);
  this->GetVarianceOutput()->Set(variance);
  this->GetSumOutput()->Set(sum);
  this->GetSumOfSquaresOutput()->Set(sumOfSquares);
  this->GetCountOutput()->Set(count);
}

template< typename TInputImage >
void
StatisticsImageCustomFilter< TInputImage >
::ThreadedGenerateData(const RegionType & outputRegionForThread,
    itk::ThreadIdType threadId)
{
  const itk::SizeValueType size0 = outputRegionForThread.GetSize(0);
  if( size0 == 0)
    {
    return;
    }
  RealType  realValue;
  PixelType value;

  RealType sum = itk::NumericTraits< RealType >::ZeroValue();
  RealType sumOfSquares = itk::NumericTraits< RealType >::ZeroValue();
  itk::SizeValueType count = itk::NumericTraits< itk::SizeValueType >::ZeroValue();
  PixelType min = itk::NumericTraits< PixelType >::max();
  PixelType max = itk::NumericTraits< PixelType >::NonpositiveMin();

  itk::ImageScanlineConstIterator< TInputImage > it (this->GetInput(),  outputRegionForThread);

  // support progress methods/callbacks
  const size_t numberOfLinesToProcess = outputRegionForThread.GetNumberOfPixels() / size0;
  itk::ProgressReporter progress( this, threadId, numberOfLinesToProcess );

  // do the work
  while ( !it.IsAtEnd() )
    {
    while ( !it.IsAtEndOfLine() )
      {
      value = it.Get();
      if (value != m_IgnoredInputValue)
        {
        realValue = static_cast< RealType >( value );
        if ( value < min )
          {
          min = value;
          }
        if ( value > max )
          {
          max  = value;
          }
        sum += realValue;
        sumOfSquares += ( realValue * realValue );
        ++count;
        }
      ++it;
      }
    it.NextLine();
    progress.CompletedPixel();
    }

  m_ThreadSum[threadId] = sum;
  m_SumOfSquares[threadId] = sumOfSquares;
  m_Count[threadId] = count;
  m_ThreadMin[threadId] = min;
  m_ThreadMax[threadId] = max;
}

template< typename TImage >
void
StatisticsImageCustomFilter< TImage >
::PrintSelf(std::ostream & os, itk::Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "Minimum: "
     << static_cast< typename itk::NumericTraits< PixelType >::PrintType >( this->GetMinimum() ) << std::endl;
  os << indent << "Maximum: "
     << static_cast< typename itk::NumericTraits< PixelType >::PrintType >( this->GetMaximum() ) << std::endl;
  os << indent << "Sum: "      << this->GetSum() << std::endl;
  os << indent << "SumOfSquares: "      << this->GetSumOfSquares() << std::endl;
  os << indent << "Count: "      << this->GetCount() << std::endl;
  os << indent << "Mean: "     << this->GetMean() << std::endl;
  os << indent << "Sigma: "    << this->GetSigma() << std::endl;
  os << indent << "Variance: " << this->GetVariance() << std::endl;
}
} // end namespace itk
#endif
