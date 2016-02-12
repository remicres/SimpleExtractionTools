/*=========================================================================

  Copyright (c) IRSTEA. All rights reserved.


     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef otbRegionComparator_H_
#define otbRegionComparator_H_

#include "otbObjectList.h"
#include "itkContinuousIndex.h"
#include "itkImageLinearConstIteratorWithIndex.h"
#include "itkMetaDataDictionary.h"
#include "ogr_api.h"
#include "ogr_feature.h"
#include "otbMetaDataKey.h"
#include "itkMetaDataObject.h"
#include "itkNumericTraits.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <algorithm>

#include "otbRemoteSensingRegion.h"

#define msg(x) do { std::cout << x << std::endl;} while(0)

using namespace std;

namespace otb {

template<class TInputImage1, class TInputImage2=TInputImage1> class RegionComparator{
public:

  typedef TInputImage1    InputImage1Type;
  typedef typename InputImage1Type::Pointer   InputImage1PointerType;
  typedef typename InputImage1Type::RegionType    InputImage1RegionType;

  typedef TInputImage2    InputImage2Type;
  typedef typename InputImage2Type::Pointer   InputImage2PointerType;
  typedef typename InputImage2Type::RegionType    InputImage2RegionType;

  typedef typename otb::RemoteSensingRegion<double> RSRegion;

  void SetImage1(InputImage1PointerType im1) {m_InputImage1 = im1;}
  void SetImage2(InputImage2PointerType im2) {m_InputImage2 = im2;}

  InputImage2RegionType GetOverlapInImage1Indices()
  {
    // Largest region of im2 --> region in im1
    InputImage1RegionType region2 = m_InputImage2->GetLargestPossibleRegion();
    InputImage1RegionType region2_in_1;
    OutputRegionToInputRegion(region2, region2_in_1, m_InputImage2, m_InputImage1);

    // Collision test
    InputImage2RegionType region1 = m_InputImage1->GetLargestPossibleRegion();
    region2_in_1.Crop(region1);

    // Return overlap
    return region2_in_1;
  }

  InputImage2RegionType GetOverlapInImage2Indices()
  {
    // Largest region of im1 --> region in im2
    InputImage1RegionType region1 = m_InputImage1->GetLargestPossibleRegion();
    InputImage1RegionType region1_in_2;
    OutputRegionToInputRegion(region1, region1_in_2, m_InputImage1, m_InputImage2);

    // Collision test
    InputImage2RegionType region2 = m_InputImage2->GetLargestPossibleRegion();
    region1_in_2.Crop(region2);

    // Return overlap
    return region1_in_2;
  }

  bool DoesOverlap()
  {
    // Largest region of im1 --> region in im2
    InputImage1RegionType region1 = m_InputImage1->GetLargestPossibleRegion();
    InputImage1RegionType region1_in_2;
    OutputRegionToInputRegion(region1, region1_in_2, m_InputImage1, m_InputImage2);

    // Collision test
    InputImage2RegionType region2 = m_InputImage2->GetLargestPossibleRegion();
    return region1_in_2.Crop(region2);
  }

  /*
   * Converts sourceRegion of im1 into targetRegion of im2
   */
  void OutputRegionToInputRegion(const InputImage1RegionType &sourceRegion, InputImage2RegionType &targetRegion,
      const InputImage1PointerType &sourceImage, const InputImage2PointerType &targetImage)
  {

    // Source Region (source image indices)
    typename InputImage1RegionType::IndexType sourceRegionIndexStart = sourceRegion.GetIndex();
    typename InputImage1RegionType::IndexType sourceRegionIndexEnd;
    for(unsigned int dim = 0; dim < InputImage1Type::ImageDimension; ++dim)
      sourceRegionIndexEnd[dim]= sourceRegionIndexStart[dim] + sourceRegion.GetSize()[dim]-1;

    // Source Region (Geo)
    typename InputImage1Type::PointType sourceRegionIndexStartGeo, sourceRegionIndexEndGeo;
    sourceImage->TransformIndexToPhysicalPoint(sourceRegionIndexStart, sourceRegionIndexStartGeo);
    sourceImage->TransformIndexToPhysicalPoint(sourceRegionIndexEnd  , sourceRegionIndexEndGeo  );

    // Source Region (target image indices)
    typename InputImage2RegionType::IndexType targetIndexStart, targetIndexEnd;
    targetImage->TransformPhysicalPointToIndex(sourceRegionIndexStartGeo, targetIndexStart);
    targetImage->TransformPhysicalPointToIndex(sourceRegionIndexEndGeo  , targetIndexEnd);

    // Target Region (target image indices)
    typename InputImage2RegionType::IndexType targetRegionStart;
    typename InputImage2RegionType::SizeType targetRegionSize;
    for(unsigned int dim = 0; dim<InputImage2Type::ImageDimension; ++dim)
      {
      targetRegionStart[dim] = std::min(targetIndexStart[dim], targetIndexEnd[dim]);
      targetRegionSize[dim]  = std::max(targetIndexStart[dim], targetIndexEnd[dim]) - targetRegionStart[dim] + 1;
      }
    InputImage2RegionType computedInputRegion(targetRegionStart, targetRegionSize);

    // Avoid extrapolation
    computedInputRegion.PadByRadius(1);

    // Target Region
    targetRegion = computedInputRegion;

  }

  /*
   * Converts a RemoteSensingREgion into a ImageRegion (image1)
   */
  InputImage1RegionType RSRegionToImageRegion(const RSRegion rsRegion)
  {
    typename itk::ContinuousIndex<double> rsRegionOrigin = rsRegion.GetOrigin();
    typename itk::ContinuousIndex<double> rsRegionSize = rsRegion.GetSize();
    typename itk::ContinuousIndex<double> rsRegionEnd;
    for(unsigned int dim = 0; dim<InputImage2Type::ImageDimension; ++dim)
      {
      rsRegionEnd[dim] = rsRegionOrigin[dim] + rsRegionSize[dim];
      }
    typename InputImage1RegionType::IndexType imageRegionStart, imageRegionEnd;
    m_InputImage1->TransformPhysicalPointToIndex(rsRegionOrigin, imageRegionStart);
    m_InputImage1->TransformPhysicalPointToIndex(rsRegionEnd, imageRegionEnd);

    // Target Region (target image indices)
    typename InputImage1RegionType::IndexType targetRegionStart;
    typename InputImage1RegionType::SizeType targetRegionSize;
    for(unsigned int dim = 0; dim<InputImage2Type::ImageDimension; ++dim)
      {
      targetRegionStart[dim] = std::min(imageRegionStart[dim], imageRegionEnd[dim]);
      targetRegionSize[dim]  = std::max(imageRegionStart[dim], imageRegionEnd[dim]) - targetRegionStart[dim] + 1;
      }

    InputImage1RegionType region(targetRegionStart, targetRegionSize);
    return region;

  }

  /*
   * Converts a given region of an image into a remoste sensing region
   */
  RSRegion ImageRegionToRSRegion(const InputImage1RegionType &sourceRegion, InputImage1PointerType sourceImage)
  {

    // Source Region (source image indices)
    typename InputImage1RegionType::IndexType sourceRegionIndexStart = sourceRegion.GetIndex();
    typename InputImage1RegionType::IndexType sourceRegionIndexEnd;
    for(unsigned int dim = 0; dim < InputImage1Type::ImageDimension; ++dim)
      sourceRegionIndexEnd[dim]= sourceRegionIndexStart[dim] + sourceRegion.GetSize()[dim]-1;

    // Source Region (Geo)
    typename InputImage1Type::PointType sourceRegionIndexStartGeo, sourceRegionIndexEndGeo;
    sourceImage->TransformIndexToPhysicalPoint(sourceRegionIndexStart, sourceRegionIndexStartGeo);
    sourceImage->TransformIndexToPhysicalPoint(sourceRegionIndexEnd  , sourceRegionIndexEndGeo  );

    // Source Region (Geo min & max)
    typename itk::ContinuousIndex<double> sourceRegionMin, sourceRegionMax, sourceRegionSize;
    for(unsigned int dim = 0; dim<InputImage2Type::ImageDimension; ++dim)
      {
      sourceRegionMin[dim] = std::min(sourceRegionIndexStartGeo[dim], sourceRegionIndexEndGeo[dim]);
      sourceRegionMax[dim] = std::max(sourceRegionIndexStartGeo[dim], sourceRegionIndexEndGeo[dim]);
      }
    RSRegion region;
    sourceRegionSize[0] = sourceRegionMax[0] - sourceRegionMin[0];
    sourceRegionSize[1] = sourceRegionMax[1] - sourceRegionMin[1];
    region.SetOrigin(sourceRegionMin);
    region.SetSize(sourceRegionSize);
    return region;

  }
  bool HaveSameProjection()
  {
    itk::MetaDataDictionary metaData1 = m_InputImage1 -> GetMetaDataDictionary();
    itk::MetaDataDictionary metaData2 = m_InputImage2 -> GetMetaDataDictionary();

    if (metaData1.HasKey(otb::MetaDataKey::ProjectionRefKey))
      {
      if (metaData2.HasKey(otb::MetaDataKey::ProjectionRefKey))
        {
        string proj1, proj2;
        itk::ExposeMetaData<string>(metaData1, static_cast<string>(otb::MetaDataKey::ProjectionRefKey), proj1);
        itk::ExposeMetaData<string>(metaData2, static_cast<string>(otb::MetaDataKey::ProjectionRefKey), proj2);
        if (proj1.compare(proj2)==0)
          return true;
        }
      }
    return false;

  }

  bool HaveSameNbOfBands()
  {
    return ( (m_InputImage1->GetNumberOfComponentsPerPixel()) == (m_InputImage2->GetNumberOfComponentsPerPixel()) );
  }

  RSRegion ComputeIntersectingRemoteSensingRegion()
  {
    InputImage2RegionType region1 = m_InputImage1->GetLargestPossibleRegion();
    InputImage1RegionType region2 = m_InputImage2->GetLargestPossibleRegion();
    RSRegion rsr1 = ImageRegionToRSRegion(region1, m_InputImage1);
    RSRegion rsr2 = ImageRegionToRSRegion(region2, m_InputImage2);
    rsr1.Crop(rsr2);
    return rsr1;
  }
private:

  InputImage1PointerType m_InputImage1;
  InputImage2PointerType m_InputImage2;

};

}

#endif /* GTBRegionComparator_H_ */
