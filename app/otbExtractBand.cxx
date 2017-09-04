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
#include "otbMultiChannelExtractROI.h"

using namespace std;

namespace otb
{

namespace Wrapper
{

class ExtractBand : public Application
{
public:
  /** Standard class typedefs. */
  typedef ExtractBand                        Self;
  typedef Application                   Superclass;
  typedef itk::SmartPointer<Self>       Pointer;
  typedef itk::SmartPointer<const Self> ConstPointer;

  typedef otb::MultiChannelExtractROI<FloatVectorImageType::InternalPixelType,
      FloatVectorImageType::InternalPixelType> ExtractFilterType;

  /** Standard macro */
  itkNewMacro(Self);
  itkTypeMacro(ExtractBand, Application);

  void DoInit()
  {

    SetName("ExtractBand");
    SetDescription("Perform single band extraction");

    // Documentation
    SetDocName("ExtractBand");
    SetDocLongDescription("This application performs single band extraction");
    SetDocLimitations("None");
    SetDocAuthors("Remi Cresson");
    SetDocSeeAlso(" ");

    AddDocTag(Tags::Manip);

    AddParameter(ParameterType_InputImage,   "inxs",  "Input XS Image");
    SetParameterDescription("inxs"," Input XS image.");

    AddParameter(ParameterType_Int, "band", "band index" );
    SetParameterDescription("band","Set the band index to extract" );
    SetMinimumParameterIntValue("band", 1);

    AddParameter(ParameterType_OutputImage,  "out",   "Output image");
    SetParameterDescription("out"," Output image.");

    AddRAMParameter();

    // Doc example parameter settings
    SetDocExampleParameterValue("band", "2");
    SetDocExampleParameterValue("inxs", "QB_Toulouse_Ortho_XS.tif");
    SetDocExampleParameterValue("out", "Pansharpening.tif uint16");


  }

  void DoUpdateParameters()
  {
    // Nothing to do here : all parameters are independent
  }

  void DoExecute()
  {

    FloatVectorImageType* xs = GetParameterImage("inxs");

    unsigned int band = GetParameterInt("band");
    m_Filter = ExtractFilterType::New();
    m_Filter->SetFirstChannel(band);
    m_Filter->SetLastChannel(band);
    m_Filter->SetInput(xs);

    SetParameterOutputImage("out", m_Filter->GetOutput());

  }

  ExtractFilterType::Pointer m_Filter;

};
}
}

OTB_APPLICATION_EXPORT( otb::Wrapper::ExtractBand )
