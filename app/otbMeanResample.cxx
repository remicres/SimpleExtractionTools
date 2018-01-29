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
#include "otbMultiToMonoChannelExtractROI.h"
#include "otbMeanResampleImageFilter.h"

using namespace std;

namespace otb
{

namespace Wrapper
{

class MeanResample : public Application
{
public:
  /** Standard class typedefs. */
  typedef MeanResample                  Self;
  typedef Application                   Superclass;
  typedef itk::SmartPointer<Self>       Pointer;
  typedef itk::SmartPointer<const Self> ConstPointer;

  typedef MultiToMonoChannelExtractROI<FloatVectorImageType::InternalPixelType,FloatVectorImageType::InternalPixelType> ExtractFilterType;

  typedef otb::MeanResampleImageFilter<FloatImageType> FilterType;

  /** Standard macro */
  itkNewMacro(Self);
  itkTypeMacro(MeanResample, Application);

  void DoInit()
  {

    SetName("MeanResample");
    SetDescription("Resample an image using the mean value of the pixels over a square neighborhood");

    // Documentation
    SetDocName("ExtractBand");
    SetDocLongDescription("This application decimates an input image using the mean value of the pixels neighborhood");
    SetDocLimitations("None");
    SetDocAuthors("Remi Cresson");
    SetDocSeeAlso(" ");

    AddDocTag(Tags::Manip);

    AddParameter(ParameterType_InputImage,   "in",  "Input XS Image");
    SetParameterDescription("in"," Input image.");

    AddParameter(ParameterType_Int, "stepx", "step x" );
    SetMinimumParameterIntValue("stepx", 2);
    SetDefaultParameterInt("stepx", 2);
    AddParameter(ParameterType_Int, "stepy", "step y" );
    SetMinimumParameterIntValue("stepy", 2);
    SetDefaultParameterInt("stepy", 2);

    AddParameter(ParameterType_OutputImage,  "out",   "Output image");
    SetParameterDescription("out"," Output image.");

    AddRAMParameter();

    // Doc example parameter settings
    SetDocExampleParameterValue("stepx", "2");
    SetDocExampleParameterValue("stepy", "2");
    SetDocExampleParameterValue("in", "QB_Toulouse_Ortho_XS.tif");
    SetDocExampleParameterValue("out", "QB_Toulouse_Ortho_XS_resampled_2x2.tif uint16");


  }

  void DoUpdateParameters()
  {
    // Nothing to do here : all parameters are independent
  }

  void DoExecute()
  {

    FloatVectorImageType* xs = GetParameterImage("in");

    m_ExtractFilter = ExtractFilterType::New();
    m_ExtractFilter->SetInput(xs);
    m_ExtractFilter->SetChannel(1);

    unsigned int stepx = GetParameterInt("stepx");
    unsigned int stepy = GetParameterInt("stepy");

    m_Filter = FilterType::New();
    m_Filter->SetStepX(stepx);
    m_Filter->SetStepY(stepy);
    m_Filter->SetInput(m_ExtractFilter->GetOutput());

    SetParameterOutputImage("out", m_Filter->GetOutput());

  }
  ExtractFilterType::Pointer m_ExtractFilter;
  FilterType::Pointer m_Filter;

};
}
}

OTB_APPLICATION_EXPORT( otb::Wrapper::MeanResample )
