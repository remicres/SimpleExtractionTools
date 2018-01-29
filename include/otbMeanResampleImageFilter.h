/*=========================================================================

  Copyright (c) Remi Cresson (IRSTEA). All rights reserved.


     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef MeanResampleImageFilter_H_
#define MeanResampleImageFilter_H_

#include "otbImage.h"
#include "itkImageToImageFilter.h"
#include "itkNumericTraits.h"
#include "itkSimpleDataObjectDecorator.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIterator.h"

// No data
#include "otbNoDataHelper.h"

namespace otb
{

/**
 * \class MeanResampleImageFilter
 * \brief This filter decimates an input image using the mean value of the pixels neighborhood.
 *
 * \ingroup TimeSeriesUtils
 */
template <class TImage>
class ITK_EXPORT MeanResampleImageFilter :
public itk::ImageToImageFilter<TImage, TImage>
{

public:

  /** Standard class typedefs. */
  typedef MeanResampleImageFilter   Self;
  typedef itk::ImageToImageFilter<TImage, TImage>	Superclass;
  typedef itk::SmartPointer<Self>                 Pointer;
  typedef itk::SmartPointer<const Self>           ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(MeanResampleImageFilter, itk::ImageToImageFilter);

  /** Iterators typedefs */
  typedef TImage ImageType;

  typedef typename ImageType::RegionType          ImageRegionType;
  typedef typename ImageType::Pointer             ImagePointer;
  typedef typename ImageType::PointType           ImagePointType;
  typedef typename ImageType::InternalPixelType   ImagePixelValueType;
  typedef typename ImageType::PixelType           ImagePixelType;
  typedef typename ImageType::IndexType           ImageIndexType;
  typedef typename ImageType::SpacingType         ImageSpacingType;
  typedef typename ImageType::SizeType            ImageSizeType;
  typedef typename itk::ImageRegionConstIterator<TImage>   InputImageIteratorType;
  typedef typename itk::ImageRegionIterator<TImage>        OutputImageIteratorType;

  itkSetMacro(NoDataValue, ImagePixelValueType);
  itkGetMacro(NoDataValue, ImagePixelValueType);

  itkSetMacro(StepX, unsigned int);
  itkSetMacro(StepY, unsigned int);

protected:
  MeanResampleImageFilter();
  virtual ~MeanResampleImageFilter() {};

  virtual void GenerateOutputInformation(void);

  virtual void GenerateInputRequestedRegion(void);

  virtual void ThreadedGenerateData(const ImageRegionType& outputRegionForThread,
      itk::ThreadIdType threadId);


private:
  MeanResampleImageFilter(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  unsigned int    m_StepX;
  unsigned int    m_StepY;

  ImagePixelValueType  m_NoDataValue;

};


} // end namespace gtb

#include <otbMeanResampleImageFilter.hxx>


#endif /* MeanResampleImageFilter_H_ */
