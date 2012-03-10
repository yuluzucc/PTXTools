/*=========================================================================
 *
 *  Copyright David Doria 2011 daviddoria@gmail.com
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include "ResectioningWidget.h"

// STL
#include <stdexcept>

// ITK
#include "itkCastImageFilter.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkRegionOfInterestImageFilter.h"
#include "itkVector.h"

// Qt
#include <QFileDialog>
#include <QIcon>
#include <QProgressDialog>
#include <QTextEdit>
#include <QtConcurrentRun>

// VTK
#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkFloatArray.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkImageSlice.h>
#include <vtkInteractorStyleImage.h>
#include <vtkLookupTable.h>
#include <vtkMath.h>
#include <vtkPointData.h>
#include <vtkPointPicker.h>
#include <vtkProperty2D.h>
#include <vtkPolyDataMapper.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkImageSliceMapper.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkXMLPolyDataReader.h>

// Custom
#include "Helpers.h"
#include "ResectioningHelpers.h"
#include "Types.h"
#include "PointSelectionStyle2D.h"
#include "PointSelectionStyle3D.h"
#include "Pane.h"
#include "Pane2D.h"
#include "Pane3D.h"
#include "PTXImage.h"
#include "PTXReader.h"

void ResectioningWidget::on_actionHelp_activated()
{
  QTextEdit* help=new QTextEdit();
  
  help->setReadOnly(true);
  help->append("<h1>Image correspondences</h1>\
  Hold the right mouse button and drag to zoom in and out. <br/>\
  Hold the middle mouse button and drag to pan the image. <br/>\
  Click the left mouse button to select a keypoint.<br/> <p/>\
  <h1>Point cloud correspondences</h1>\
  Hold the left mouse button and drag to rotate the scene.<br/>\
  Hold the right mouse button and drag to zoom in and out. Hold the middle mouse button and drag to pan the scene. While holding control (CTRL), click the left mouse button to select a keypoint.<br/>\
  If you need to zoom in farther, hold shift while left clicking a point to change the camera's focal point to that point. You can reset the focal point by pressing 'r'.\
  <h1>Saving keypoints</h1>\
  The same number of keypoints must be selected in both the left and right panels before the points can be saved."
  );
  help->show();
}

void ResectioningWidget::on_actionQuit_activated()
{
  exit(0);
}

void ResectioningWidget::SharedConstructor()
{
  this->setupUi(this);

  connect(&this->FutureWatcher, SIGNAL(finished()), this->ProgressDialog , SLOT(cancel()));

  PointCloudPane = new Pane3D(this->qvtkWidgetLeft);
  ImagePane = new Pane2D(this->qvtkWidgetRight);

  ProgressDialog = new QProgressDialog;
}

ResectioningWidget::ResectioningWidget(const std::string& imageFileName, const std::string& pointCloudFileName)
{
  SharedConstructor();

  LoadImage(imageFileName);

  LoadPTX(pointCloudFileName);
}

// Constructor
ResectioningWidget::ResectioningWidget()
{
  SharedConstructor();
};

void ResectioningWidget::LoadCorrespondencesImage(const std::string& filename)
{
  std::string line;
  std::ifstream fin(filename.c_str());

  if(fin == NULL)
    {
    throw std::runtime_error("LoadCorrespondences2D Cannot open file.");
    }

  ImagePane->SelectionStyle->RemoveAll();

  while(getline(fin, line))
    {
    std::stringstream ss;
    ss << line;
    double p[3];
    ss >> p[0] >> p[1];
    p[2] = 0;

    ImagePane->SelectionStyle->AddNumber(p);
    }
}

void ResectioningWidget::LoadCorrespondencesPointCloud(const std::string& filename)
{
  std::string line;
  std::ifstream fin(filename.c_str());

  if(fin == NULL)
    {
    throw std::runtime_error("LoadCorrespondences3D Cannot open file.");
    }

  PointCloudPane->SelectionStyle->RemoveAll();

  while(getline(fin, line))
    {
    std::stringstream ss;
    ss << line;
    double p[3];
    ss >> p[0] >> p[1] >> p[2];

    PointCloudPane->SelectionStyle->AddNumber(p);
    }
}


void ResectioningWidget::SaveCorrespondencesImage(const std::string& filename)
{
  std::ofstream fout(filename.c_str());

  for(unsigned int i = 0; i < ImagePane->SelectionStyle->GetNumberOfCorrespondences(); i++)
    {
    fout << ImagePane->SelectionStyle->GetCorrespondence(i).x << " "
         << ImagePane->SelectionStyle->GetCorrespondence(i).y << std::endl;

    }
  fout.close();
}

void ResectioningWidget::SaveCorrespondencesPointCloud(const std::string& filename)
{
  std::ofstream fout(filename.c_str());

  for(unsigned int i = 0; i < PointCloudPane->SelectionStyle->GetNumberOfCorrespondences(); i++)
    {
    fout << PointCloudPane->SelectionStyle->GetCorrespondence(i).x << " "
         << PointCloudPane->SelectionStyle->GetCorrespondence(i).y << " "
         << PointCloudPane->SelectionStyle->GetCorrespondence(i).z << std::endl;
    }
  fout.close();
}

void ResectioningWidget::LoadImage(const std::string& fileName)
{

/*
  QFileInfo fileInfo(fileName.toStdString().c_str());
  std::string extension = fileInfo.suffix().toStdString();
  std::cout << "extension: " << extension << std::endl;*/

  typedef itk::ImageFileReader<FloatVectorImageType> ReaderType;
  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName(fileName);
  reader->Update();

  ImagePane->Image = reader->GetOutput();

  if(this->chkRGB->isChecked())
    {
    ResectioningHelpers::ITKImagetoVTKRGBImage(ImagePane->Image.GetPointer(), ImagePane->ImageData);
    }
  else
    {
    ResectioningHelpers::ITKImagetoVTKMagnitudeImage(ImagePane->Image.GetPointer(), ImagePane->ImageData);
    }

  ImagePane->ImageSliceMapper->SetInputConnection(ImagePane->ImageData->GetProducerPort());
  ImagePane->ImageSlice->SetMapper(ImagePane->ImageSliceMapper);
  
  // Add Actor to renderer
  //pane->Renderer->AddActor(pane->ImageActor);
  ImagePane->Renderer->AddActor(ImagePane->ImageSlice);
  ImagePane->Renderer->ResetCamera();

  vtkSmartPointer<vtkPointPicker> pointPicker = vtkSmartPointer<vtkPointPicker>::New();
  ImagePane->qvtkWidget->GetRenderWindow()->GetInteractor()->SetPicker(pointPicker);
  ImagePane->SelectionStyle = PointSelectionStyle2D::New();
  ImagePane->SelectionStyle->SetCurrentRenderer(ImagePane->Renderer);
  //pane->SelectionStyle->Initialize();
  ImagePane->qvtkWidget->GetRenderWindow()->GetInteractor()->
            SetInteractorStyle(static_cast<PointSelectionStyle2D*>(ImagePane->SelectionStyle));

  ImagePane->Renderer->ResetCamera();

  ImagePane->qvtkWidget->GetRenderWindow()->Render();
}

void ResectioningWidget::on_action_Image_FlipHorizontally_activated()
{
  ImagePane->FlipHorizontally();
}

void ResectioningWidget::on_action_Image_FlipVertically_activated()
{
  ImagePane->FlipVertically();
}

void ResectioningWidget::LoadPTX(const std::string& fileName)
{
  PTXReader reader;
  reader.SetFileName(fileName.c_str());

  // Read in the same thread
//   reader.Read();
//   PTXImage ptxImage = reader.GetOutput();

  // Start the computation.
  QFuture<void> future = QtConcurrent::run(&reader, &PTXReader::Read);
  this->FutureWatcher.setFuture(future);
  this->ProgressDialog->setMinimum(0);
  this->ProgressDialog->setMaximum(0);
  this->ProgressDialog->setLabelText("Opening PTX file...");
  this->ProgressDialog->setWindowModality(Qt::WindowModal);
  this->ProgressDialog->exec();

  PTXImage ptxImage = reader.GetOutput();

  vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
  ptxImage.CreatePointCloud(polyData);

  vtkSmartPointer<vtkLookupTable> lookupTable = vtkSmartPointer<vtkLookupTable>::New();
  //lookupTable->SetTableRange(0.0, 10.0);
  //lookupTable->SetHueRange(0, .5);
  //lookupTable->SetHueRange(.5, 1);
  lookupTable->SetHueRange(0, 1);

  // If the cloud has an "Intensity" array, use it for the initial coloring
  vtkFloatArray* intensityArray = vtkFloatArray::SafeDownCast(
                                  polyData->GetPointData()->GetArray("Intensity"));
  if(intensityArray)
    {
    polyData->GetPointData()->SetActiveScalars("Intensity");

    float range[2];
    intensityArray->GetValueRange(range);

    lookupTable->SetTableRange(range[0], range[1]);
    }

  PointCloudPane->PointCloudMapper->SetInputConnection(polyData->GetProducerPort());
  PointCloudPane->PointCloudMapper->SetLookupTable(lookupTable);
  PointCloudPane->PointCloudActor->SetMapper(PointCloudPane->PointCloudMapper);
  PointCloudPane->PointCloudActor->GetProperty()->SetRepresentationToPoints();

  // Add Actor to renderer
  PointCloudPane->Renderer->AddActor(PointCloudPane->PointCloudActor);
  PointCloudPane->Renderer->ResetCamera();

  vtkSmartPointer<vtkPointPicker> pointPicker = vtkSmartPointer<vtkPointPicker>::New();

  pointPicker->PickFromListOn();
  pointPicker->AddPickList(PointCloudPane->PointCloudActor);
  PointCloudPane->qvtkWidget->GetRenderWindow()->GetInteractor()->SetPicker(pointPicker);
  PointCloudPane->SelectionStyle = PointSelectionStyle3D::New();
  PointCloudPane->SelectionStyle->SetCurrentRenderer(PointCloudPane->Renderer);
  PointCloudPane->SelectionStyle->Initialize();
  static_cast<PointSelectionStyle3D*>(PointCloudPane->SelectionStyle)->Data = polyData;
  PointCloudPane->qvtkWidget->GetRenderWindow()->GetInteractor()->SetInteractorStyle(
         static_cast<PointSelectionStyle3D*>(PointCloudPane->SelectionStyle));

  PointCloudPane->Renderer->ResetCamera();

  std::cout << "Computing average spacing..." << std::endl;
  float averageSpacing = ResectioningHelpers::ComputeAverageSpacing(polyData->GetPoints(), 100000);
  std::cout << "Done computing average spacing: " << averageSpacing << std::endl;

  static_cast<PointSelectionStyle3D*>(PointCloudPane->SelectionStyle)->SetMarkerRadius(averageSpacing * 10.0);
}

void ResectioningWidget::LoadVTP(const std::string& fileName)
{

/*
  QFileInfo fileInfo(fileName.toStdString().c_str());
  std::string extension = fileInfo.suffix().toStdString();
  std::cout << "extension: " << extension << std::endl;
  */

  vtkSmartPointer<vtkXMLPolyDataReader> reader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
  reader->SetFileName(fileName.c_str());

  //reader->Update();
  // Start the computation.
  QFuture<void> future = QtConcurrent::run(reader.GetPointer(),
                            static_cast<void(vtkXMLPolyDataReader::*)()>(&vtkXMLPolyDataReader::Update));
  this->FutureWatcher.setFuture(future);
  this->ProgressDialog->setMinimum(0);
  this->ProgressDialog->setMaximum(0);
  this->ProgressDialog->setLabelText("Opening file...");
  this->ProgressDialog->setWindowModality(Qt::WindowModal);
  this->ProgressDialog->exec();

  vtkSmartPointer<vtkLookupTable> lookupTable = vtkSmartPointer<vtkLookupTable>::New();
  //lookupTable->SetTableRange(0.0, 10.0);
  //lookupTable->SetHueRange(0, .5);
  //lookupTable->SetHueRange(.5, 1);
  lookupTable->SetHueRange(0, 1);

  // If the cloud has an "Intensity" array, use it for the initial coloring
  vtkFloatArray* intensityArray = vtkFloatArray::SafeDownCast(
                                  reader->GetOutput()->GetPointData()->GetArray("Intensity"));
  if(intensityArray)
    {
    reader->GetOutput()->GetPointData()->SetActiveScalars("Intensity");

    float range[2];
    intensityArray->GetValueRange(range);

    lookupTable->SetTableRange(range[0], range[1]);
    }

  PointCloudPane->PointCloudMapper->SetInputConnection(reader->GetOutputPort());
  PointCloudPane->PointCloudMapper->SetLookupTable(lookupTable);
  PointCloudPane->PointCloudActor->SetMapper(PointCloudPane->PointCloudMapper);
  PointCloudPane->PointCloudActor->GetProperty()->SetRepresentationToPoints();

  // Add Actor to renderer
  PointCloudPane->Renderer->AddActor(PointCloudPane->PointCloudActor);
  PointCloudPane->Renderer->ResetCamera();

  vtkSmartPointer<vtkPointPicker> pointPicker = vtkSmartPointer<vtkPointPicker>::New();

  pointPicker->PickFromListOn();
  pointPicker->AddPickList(PointCloudPane->PointCloudActor);
  PointCloudPane->qvtkWidget->GetRenderWindow()->GetInteractor()->SetPicker(pointPicker);
  PointCloudPane->SelectionStyle = PointSelectionStyle3D::New();
  PointCloudPane->SelectionStyle->SetCurrentRenderer(PointCloudPane->Renderer);
  PointCloudPane->SelectionStyle->Initialize();
  static_cast<PointSelectionStyle3D*>(PointCloudPane->SelectionStyle)->Data = reader->GetOutput();
  PointCloudPane->qvtkWidget->GetRenderWindow()->GetInteractor()->SetInteractorStyle(
         static_cast<PointSelectionStyle3D*>(PointCloudPane->SelectionStyle));

  PointCloudPane->Renderer->ResetCamera();

  std::cout << "Computing average spacing..." << std::endl;
  float averageSpacing = ResectioningHelpers::ComputeAverageSpacing(reader->GetOutput()->GetPoints(), 100000);
  std::cout << "Done computing average spacing: " << averageSpacing << std::endl;

  static_cast<PointSelectionStyle3D*>(PointCloudPane->SelectionStyle)->SetMarkerRadius(averageSpacing * 10.0);
}

void ResectioningWidget::on_action_Image_LoadCorrespondences_activated()
{
  // Get a filename to open
  QString fileName = QFileDialog::getOpenFileName(this, "Open File", ".",
                                                  "Image Files (*.jpg *.jpeg *.bmp *.png *.mha)");

  std::cout << "Got filename: " << fileName.toStdString() << std::endl;
  if(fileName.toStdString().empty())
    {
    std::cout << "Filename was empty." << std::endl;
    return;
    }

  LoadCorrespondencesImage(fileName.toStdString());
}

void ResectioningWidget::on_action_PointCloud_LoadCorrespondences_activated()
{
  // Get a filename to open
  QString fileName = QFileDialog::getOpenFileName(this, "Open File", ".",
                                                  "Image Files (*.jpg *.jpeg *.bmp *.png *.mha)");

  std::cout << "Got filename: " << fileName.toStdString() << std::endl;
  if(fileName.toStdString().empty())
    {
    std::cout << "Filename was empty." << std::endl;
    return;
    }
  LoadCorrespondencesPointCloud(fileName.toStdString());
}

void ResectioningWidget::on_action_Image_Open_activated()
{
  // Get a filename to open
  QString fileName = QFileDialog::getOpenFileName(this, "Open File", ".",
                                                  "Image Files (*.jpg *.jpeg *.bmp *.png *.mha)");

  std::cout << "Got filename: " << fileName.toStdString() << std::endl;
  if(fileName.toStdString().empty())
    {
    std::cout << "Filename was empty." << std::endl;
    return;
    }

  LoadImage(fileName.toStdString());
}

void ResectioningWidget::on_action_PointCloud_OpenVTP_activated()
{
  // Get a filename to open
  QString fileName = QFileDialog::getOpenFileName(this, "Open Point Cloud", ".", "Files (*.vtp)");

  std::cout << "Got filename: " << fileName.toStdString() << std::endl;
  if(fileName.toStdString().empty())
    {
    std::cout << "Filename was empty." << std::endl;
    return;
    }

  LoadVTP(fileName.toStdString());
  std::cout << "Done loading point cloud." << std::endl;
}

void ResectioningWidget::on_action_PointCloud_OpenPTX_activated()
{
  // Get a filename to open
  QString fileName = QFileDialog::getOpenFileName(this, "Open Point Cloud", ".", "Files (*.vtp)");

  std::cout << "Got filename: " << fileName.toStdString() << std::endl;
  if(fileName.toStdString().empty())
    {
    std::cout << "Filename was empty." << std::endl;
    return;
    }

  LoadPTX(fileName.toStdString());
  std::cout << "Done loading point cloud." << std::endl;
}


void ResectioningWidget::on_action_Image_SaveCorrespondences_activated()
{
  QString fileName = QFileDialog::getSaveFileName(this, "Save File", ".", "Text Files (*.txt)");
  std::cout << "Got filename: " << fileName.toStdString() << std::endl;
  if(fileName.toStdString().empty())
    {
    std::cout << "Filename was empty." << std::endl;
    return;
    }
  
}

void ResectioningWidget::on_action_PointCloud_SaveCorrespondences_activated()
{
  QString fileName = QFileDialog::getSaveFileName(this, "Save File", ".", "Text Files (*.txt)");
  std::cout << "Got filename: " << fileName.toStdString() << std::endl;
  if(fileName.toStdString().empty())
    {
    std::cout << "Filename was empty." << std::endl;
    return;
    }
}

void ResectioningWidget::on_btnDeleteLastCorrespondencePointCloud_clicked()
{
  this->PointCloudPane->SelectionStyle->DeleteLastCorrespondence();
  this->PointCloudPane->Refresh();
}

void ResectioningWidget::on_btnDeleteAllCorrespondencesPointCloud_clicked()
{
  this->PointCloudPane->SelectionStyle->RemoveAll();
  this->PointCloudPane->Refresh();
  //static_cast<PointSelectionStyle2D*>(pane->SelectionStyle)->RemoveAll();
  //this->qvtkWidgetLeft->GetRenderWindow()->Render();
}

void ResectioningWidget::on_btnDeleteLastCorrespondenceImage_clicked()
{
  this->ImagePane->SelectionStyle->DeleteLastCorrespondence();
  this->ImagePane->Refresh();
}

void ResectioningWidget::on_btnDeleteAllCorrespondencesImage_clicked()
{
  this->ImagePane->SelectionStyle->RemoveAll();
  this->ImagePane->Refresh();
  //static_cast<PointSelectionStyle2D*>(pane->SelectionStyle)->RemoveAll();
  //this->qvtkWidgetRight->GetRenderWindow()->Render();
}