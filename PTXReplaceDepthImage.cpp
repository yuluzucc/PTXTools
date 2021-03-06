#include "PTXImage.h"
#include "PTXReader.h"

#include "itkImageFileReader.h"

int main (int argc, char *argv[])
{
  if(argc != 4)
    {
    std::cout << "Required arguments: InputFilename(ptx) NewDepthImage(mhd) OutputFilename(ptx)" << std::endl;
    return EXIT_FAILURE;
    }

  std::string inputFilename = argv[1];
  std::string newDepthImageFilename = argv[2];
  std::string outputFilename = argv[3];

  PTXImage ptxImage = PTXReader::Read(inputFilename);

  typedef itk::Image<float, 2> FloatImageType;
  typedef itk::ImageFileReader<FloatImageType> FloatImageReaderType;
  FloatImageReaderType::Pointer reader = FloatImageReaderType::New();
  reader->SetFileName(newDepthImageFilename);
  reader->Update();

  ptxImage.ReplaceDepth(reader->GetOutput());

  ptxImage.WritePTX(outputFilename);

  return EXIT_SUCCESS;
}
