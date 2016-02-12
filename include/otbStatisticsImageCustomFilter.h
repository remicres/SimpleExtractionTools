/*=========================================================================

  Copyright (c) IRSTEA. All rights reserved.

Some parts of this code are derived from ITK. See ITKCopyright.txt
for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __otbStatisticsImageCustomFilter_h
#define __otbStatisticsImageCustomFilter_h

#include "itkImageToImageFilter.h"
#include "itkNumericTraits.h"
#include "itkArray.h"
#include "itkSimpleDataObjectDecorator.h"

namespace otb
{
/** \class StatisticsImageCustomFilter
 * \brief Compute min. max, variance and mean of an Image.
 *
 * StatisticsImageCustomFilter computes the minimum, maximum, sum, mean, variance
 * sigma of an image.  The filter needs all of its input image.  It
 * behaves as a filter with an input and output. Thus it can be inserted
 * in a pipline with other filters and the statistics will only be
 * recomputed if a downstream filter changes.
 *
 * The filter passes its input through unmodified.  The filter is
 * threaded. It computes statistics in each thread then combines them in
 * its AfterThreadedGenerate method.
 *
 * A value to ignore (in the stats) can be chosen
 *
 * \ingroup MathematicalStatisticsImageCustomFilters
 * \ingroup ITKImageStatistics
 *
 * \wiki
 * \wikiexample{Statistics/StatisticsImageCustomFilter,Compute min\, max\, variance and mean of an Image.}
 * \endwiki
 */
template< typename TInputImage >
class StatisticsImageCustomFilter:
  public itk::ImageToImageFilter< TInputImage, TInputImage >
{
public:
  /** Standard Self typedef */
  typedef StatisticsImageCustomFilter                          Self;
  typedef itk::ImageToImageFilter< TInputImage, TInputImage > Superclass;
  typedef itk::SmartPointer< Self >                           Pointer;
  typedef itk::SmartPointer< const Self >                     ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Runtime information support. */
  itkTypeMacro(StatisticsImageCustomFilter,itk::ImageToImageFilter);

  /** Image related typedefs. */
  typedef typename TInputImage::Pointer InputImagePointer;

  typedef typename TInputImage::RegionType RegionType;
  typedef typename TInputImage::SizeType   SizeType;
  typedef typename TInputImage::IndexType  IndexType;
  typedef typename TInputImage::PixelType  PixelType;

  /** Image related typedefs. */
  itkStaticConstMacro(ImageDimension, unsigned int,
                      TInputImage::ImageDimension);

  /** Type to use for computations. */
  typedef typename itk::NumericTraits< PixelType >::RealType RealType;
  typedef unsigned long LongType;

  /** Smart Pointer type to a DataObject. */
  typedef typename itk::DataObject::Pointer DataObjectPointer;

  /** Type of DataObjects used for scalar outputs */
  typedef itk::SimpleDataObjectDecorator< RealType >  RealObjectType;
  typedef itk::SimpleDataObjectDecorator< LongType >  LongObjectType;
  typedef itk::SimpleDataObjectDecorator< PixelType > PixelObjectType;

  /** Return the computed Minimum. */
  PixelType GetMinimum() const
  { return this->GetMinimumOutput()->Get(); }
  PixelObjectType * GetMinimumOutput();

  const PixelObjectType * GetMinimumOutput() const;

  /** Return the computed Maximum. */
  PixelType GetMaximum() const
  { return this->GetMaximumOutput()->Get(); }
  PixelObjectType * GetMaximumOutput();

  const PixelObjectType * GetMaximumOutput() const;

  /** Return the computed Mean. */
  RealType GetMean() const
  { return this->GetMeanOutput()->Get(); }
  RealObjectType * GetMeanOutput();

  const RealObjectType * GetMeanOutput() const;

  /** Return the computed Standard Deviation. */
  RealType GetSigma() const
  { return this->GetSigmaOutput()->Get(); }
  RealObjectType * GetSigmaOutput();

  const RealObjectType * GetSigmaOutput() const;

  /** Return the computed Variance. */
  RealType GetVariance() const
  { return this->GetVarianceOutput()->Get(); }
  RealObjectType * GetVarianceOutput();

  const RealObjectType * GetVarianceOutput() const;

  /** Return the compute Sum. */
  RealType GetSum() const
  { return this->GetSumOutput()->Get(); }
  RealObjectType * GetSumOutput();

  const RealObjectType * GetSumOutput() const;

  /** Return the compute Sum of squares. */
  RealType GetSumOfSquares() const
  { return this->GetSumOfSquaresOutput()->Get(); }
  RealObjectType * GetSumOfSquaresOutput();

  const RealObjectType * GetSumOfSquaresOutput() const;

  /** Return the compute count. */
  RealType GetCount() const
  { return this->GetCountOutput()->Get(); }
  LongObjectType * GetCountOutput();

  const LongObjectType * GetCountOutput() const;

  /** Make a DataObject of the correct type to be used as the specified
   * output. */
  typedef itk::ProcessObject::DataObjectPointerArraySizeType DataObjectPointerArraySizeType;
  using Superclass::MakeOutput;
  virtual DataObjectPointer MakeOutput(DataObjectPointerArraySizeType idx);

#ifdef ITK_USE_CONCEPT_CHECKING
  // Begin concept checking
  itkConceptMacro( InputHasNumericTraitsCheck,
                   ( itk::Concept::HasNumericTraits< PixelType > ) );
  // End concept checking
#endif

  /** Accessors for ignored value */
  itkGetMacro(IgnoredInputValue, PixelType);
  itkSetMacro(IgnoredInputValue, PixelType);

protected:
  StatisticsImageCustomFilter();
  ~StatisticsImageCustomFilter(){}
  void PrintSelf(std::ostream & os, itk::Indent indent) const;

  /** Pass the input through unmodified. Do this by Grafting in the
   *  AllocateOutputs method.
   */
  void AllocateOutputs();

  /** Initialize some accumulators before the threads run. */
  void BeforeThreadedGenerateData();

  /** Do final mean and variance computation from data accumulated in threads.
   */
  void AfterThreadedGenerateData();

  /** Multi-thread version GenerateData. */
  void  ThreadedGenerateData(const RegionType &
                             outputRegionForThread,
                             itk::ThreadIdType threadId);

private:
  StatisticsImageCustomFilter(const Self &); //purposely not implemented
  void operator=(const Self &);        //purposely not implemented

  itk::Array< RealType >       m_ThreadSum;
  itk::Array< RealType >       m_SumOfSquares;
  itk::Array< itk::SizeValueType >  m_Count;
  itk::Array< PixelType >      m_ThreadMin;
  itk::Array< PixelType >      m_ThreadMax;

  PixelType m_IgnoredInputValue;
}; // end of class
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "otbStatisticsImageCustomFilter.hxx"
#endif

#endif
