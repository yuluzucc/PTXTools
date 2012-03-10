/*
Copyright (C) 2010 David Doria, daviddoria@gmail.com

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef HELPERS_HPP
#define HELPERS_HPP

// ITK
#include "itkRescaleIntensityImageFilter.h"
#include "itkImageFileWriter.h"

// VTK
#include <vtkImageData.h>

// STL
#include <stdexcept>

namespace Helpers
{

template<typename TImage>
void WriteImage(const TImage* const image, const std::string& filename)
{
  // This is a convenience function so that images can be written in 1 line instead of 4.
  typename itk::ImageFileWriter<TImage>::Pointer writer = itk::ImageFileWriter<TImage>::New();
  writer->SetFileName(filename);
  writer->SetInput(image);
  writer->Update();
}

/** Copy the input to the output*/
template<typename TImage>
void DeepCopy(const TImage* const input, TImage* const output)
{
  output->SetRegions(input->GetLargestPossibleRegion());
  output->Allocate();

  itk::ImageRegionConstIterator<TImage> inputIterator(input, input->GetLargestPossibleRegion());
  itk::ImageRegionIterator<TImage> outputIterator(output, output->GetLargestPossibleRegion());

  while(!inputIterator.IsAtEnd())
    {
    outputIterator.Set(inputIterator.Get());
    ++inputIterator;
    ++outputIterator;
    }
}

template<typename TImage>
void SetAllPixelsToValue(TImage* const image, typename TImage::PixelType& pixel)
{
  itk::ImageRegionIterator<TImage> imageIterator(image, image->GetLargestPossibleRegion());

  while(!imageIterator.IsAtEnd())
    {
    imageIterator.Set(pixel);
    ++imageIterator;
    }
}


template <typename TImage>
void ITKScalarImageToScaledVTKImage(const TImage* const image, vtkImageData* const outputImage)
{
  //std::cout << "ITKScalarImagetoVTKImage()" << std::endl;
  typedef itk::Image<unsigned char, 2> UnsignedCharScalarImageType;

  // Rescale and cast for display
  typedef itk::RescaleIntensityImageFilter<TImage, UnsignedCharScalarImageType> RescaleFilterType;
  typename RescaleFilterType::Pointer rescaleFilter = RescaleFilterType::New();
  rescaleFilter->SetOutputMinimum(0);
  rescaleFilter->SetOutputMaximum(255);
  rescaleFilter->SetInput(image);
  rescaleFilter->Update();

  // Setup and allocate the VTK image
  outputImage->SetNumberOfScalarComponents(1);
  outputImage->SetScalarTypeToUnsignedChar();
  outputImage->SetDimensions(image->GetLargestPossibleRegion().GetSize()[0],
                             image->GetLargestPossibleRegion().GetSize()[1],
                             1);

  outputImage->AllocateScalars();

  // Copy all of the scaled magnitudes to the output image
  itk::ImageRegionConstIteratorWithIndex<UnsignedCharScalarImageType> imageIterator(rescaleFilter->GetOutput(), rescaleFilter->GetOutput()->GetLargestPossibleRegion());
  imageIterator.GoToBegin();

  while(!imageIterator.IsAtEnd())
    {
    unsigned char* pixel = static_cast<unsigned char*>(outputImage->GetScalarPointer(imageIterator.GetIndex()[0],
                                                                                     imageIterator.GetIndex()[1],0));
    pixel[0] = imageIterator.Get();

    ++imageIterator;
    }

  outputImage->Modified();
}

template<typename TImage>
itk::Index<2> GetNearestValidPixel(const TImage* const image, const itk::Index<2>& queryPixel)
{
  throw std::runtime_error("Helpers::GetNearestValidPixel() is not yet implemented!");
}

} // end namespace

#endif