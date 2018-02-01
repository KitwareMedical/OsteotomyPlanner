// My library
#include "OsteotomyModelToModelDistanceCLP.h"
#include <vtkDistancePolyDataFilter.h>
#include <vtkVersion.h>
#include <vtkPolyDataWriter.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkPolyDataReader.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkPolyData.h>
#include <vtkCommand.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>
#include <vtkCleanPolyData.h>
#include <vtkTriangleFilter.h>
#include <vtkCellData.h>
#include <vtkPointData.h>
#include <vtkDoubleArray.h>
#include <vtkMath.h>
#include <vtkPolyDataNormals.h>
#include <vtkFloatArray.h>
#include <vtksys/SystemTools.hxx>

//class ErrorObserver copied from http://www.vtk.org/Wiki/VTK/Examples/Cxx/Utilities/ObserveError
class ErrorObserver : public vtkCommand
{
public:
    ErrorObserver():
        Error(false),
        Warning(false),
        ErrorMessage(""),
        WarningMessage("") {}
    static ErrorObserver *New()
    {
        return new ErrorObserver;
    }
    bool GetError() const
    {
        return this->Error;
    }
    bool GetWarning() const
    {
        return this->Warning;
    }
    void Clear()
    {
        this->Error = false;
        this->Warning = false;
        this->ErrorMessage = "";
        this->WarningMessage = "";
    }
    virtual void Execute(vtkObject *vtkNotUsed(caller),
                         unsigned long event,
                         void *calldata)
    {
        switch(event)
        {
        case vtkCommand::ErrorEvent:
            ErrorMessage = static_cast<char *>(calldata);
            this->Error = true;
            break;
        case vtkCommand::WarningEvent:
            WarningMessage = static_cast<char *>(calldata);
            this->Warning = true;
            break;
        }
    }
    std::string GetErrorMessage()
    {
        return ErrorMessage;
    }
    std::string GetWarningMessage()
    {
        return WarningMessage;
    }
private:
    bool        Error;
    bool        Warning;
    std::string ErrorMessage;
    std::string WarningMessage;
};


int TriangulateAndClean(vtkSmartPointer<vtkPolyData> &polyData )
{
    vtkSmartPointer <vtkCleanPolyData> Cleaner = vtkSmartPointer <vtkCleanPolyData>::New() ;
    Cleaner->SetInputData( polyData ) ;
    Cleaner->Update() ;
    vtkSmartPointer <vtkTriangleFilter> Triangler = vtkSmartPointer <vtkTriangleFilter>::New() ;
    Triangler->SetInputData( Cleaner->GetOutput() ) ;
    Triangler->Update() ;
    polyData = Triangler->GetOutput() ;
    return 0 ;
}


void PointsToVec( double p1[] , double p2[] , double vec[] )
{
    for( int i = 0 ; i < 3 ; i++ )
    {
        vec[ i ] = p2[ i ] - p1[ i ] ;
    }
}



int ClosestPointDistance( vtkSmartPointer< vtkPolyData > &inPolyData1 ,
                          vtkSmartPointer< vtkPolyData > &inPolyData2 ,
                          bool signedDistance ,
                          vtkSmartPointer< vtkPolyData > &outPolyData ,
                          std::string outputFieldSuffix
                          )
{
    vtkSmartPointer<vtkDistancePolyDataFilter> distanceFilter =
            vtkSmartPointer<vtkDistancePolyDataFilter>::New();
    distanceFilter->SetInputData( 0, inPolyData1 ) ;
    distanceFilter->SetInputData( 1, inPolyData2 ) ;
    distanceFilter->SetSignedDistance( signedDistance ) ;
    distanceFilter->Update();
    //We are only interested in the point distance, not in the cell distance, so we remove the cell distance
    distanceFilter->GetOutput()->GetCellData()->RemoveArray("Distance") ;
    //We rename the output array name to match the expected names in 3DMeshMetric
    std::string distanceName ;
    if( signedDistance )
    {
        distanceName = "Signed" + outputFieldSuffix ;
    }
    else
    {
        distanceName = "Absolute" + outputFieldSuffix;
    }
    distanceFilter->GetOutput()->GetPointData()->GetArray("Distance")->SetName(distanceName.c_str()) ;
    //We add a constant field that we call "original" that allows to show easily the model with no color map (constant color)
    vtkSmartPointer<vtkPolyData> distancePolyData = distanceFilter->GetOutput() ;
    vtkSmartPointer <vtkDoubleArray> ScalarsConst = vtkSmartPointer <vtkDoubleArray>::New();
    for( vtkIdType Id = 0 ; Id < distancePolyData->GetNumberOfPoints() ; Id++ )
    {
        ScalarsConst->InsertTuple1( Id , 1.0 );
    }
    ScalarsConst->SetName( "Original" );
    distancePolyData->GetPointData()->AddArray( ScalarsConst );
    vtkSmartPointer <vtkCleanPolyData> Cleaner = vtkSmartPointer <vtkCleanPolyData>::New() ;
    Cleaner->SetInputData( distancePolyData ) ;
    Cleaner->Update() ;
    outPolyData = Cleaner->GetOutput() ;
    return 0 ;
}

int ReadVTK( std::string input , vtkSmartPointer<vtkPolyData> &polyData )
{
    vtkSmartPointer<ErrorObserver>  errorObserver =
            vtkSmartPointer<ErrorObserver>::New();
    if( input.rfind( ".vtk" ) != std::string::npos )
    {
      vtkSmartPointer< vtkPolyDataReader > polyReader = vtkSmartPointer<vtkPolyDataReader>::New();
        polyReader->AddObserver( vtkCommand::ErrorEvent , errorObserver ) ;
        polyReader->SetFileName( input.c_str() ) ;
        polyData = polyReader->GetOutput() ;
        polyReader->Update() ;
    }
    else if( input.rfind( ".vtp" ) != std::string::npos )
    {
      vtkSmartPointer< vtkXMLPolyDataReader > xmlReader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
        xmlReader->SetFileName( input.c_str() ) ;
        xmlReader->AddObserver( vtkCommand::ErrorEvent , errorObserver ) ;
        polyData = xmlReader->GetOutput() ;
        xmlReader->Update() ;
    }
    else
    {
        std::cerr << "Input file format not handled: " << input << " cannot be read" << std::endl ;
        return 1 ;
    }
    if (errorObserver->GetError())
    {
        std::cout << "Caught error opening " << input << std::endl ;
        return 1 ;
    }
    return 0 ;
}

int WriteVTK( std::string output , vtkSmartPointer<vtkPolyData> &polyData )
{
    vtkSmartPointer<ErrorObserver>  errorObserver =
            vtkSmartPointer<ErrorObserver>::New();
    if (output.rfind(".vtk") != std::string::npos )
    {
        vtkSmartPointer<vtkPolyDataWriter> writer = vtkSmartPointer<vtkPolyDataWriter>::New() ;
        writer->SetFileName( output.c_str() ) ;
        writer->AddObserver( vtkCommand::ErrorEvent , errorObserver ) ;
        writer->SetInputData( polyData ) ;
        writer->Update();
    }
    else if( output.rfind( ".vtp" ) != std::string::npos )
    {
      vtkSmartPointer< vtkXMLPolyDataWriter > writer = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
        writer->SetFileName( output.c_str() ) ;
        writer->AddObserver( vtkCommand::ErrorEvent , errorObserver ) ;
        writer->SetInputData( polyData ) ;
        writer->Update() ;
    }
    else
    {
        std::cerr << "Output file format not handled: " << output << " cannot be written" << std::endl ;
        return 1 ;
    }
    if (errorObserver->GetError())
    {
        std::cout << "Caught error saving " << output << std::endl ;
        return 1 ;
    }
    return 0 ;
}

int main( int argc , char* argv[] )
{
    PARSE_ARGS ;
    if( vtkFile1.empty() || vtkFile2.empty() )
    {
        std::cout << "Specify 2 vtk input files" << std::endl ;
        return 1 ;
    }
    if( vtkOutput.empty() )
    {
        std::cout << "Specify an output file name" << std::endl ;
        return 1 ;
    }
    vtkSmartPointer<vtkPolyData> inPolyData1 = vtkSmartPointer<vtkPolyData>::New() ;
    if( ReadVTK( vtkFile1 , inPolyData1 ) )
    {
        return 1 ;
    }
    vtkSmartPointer<vtkPolyData> inPolyData2 = vtkSmartPointer<vtkPolyData>::New() ;
    if( ReadVTK( vtkFile2 , inPolyData2 ) )
    {
        return 1 ;
    }
    if( TriangulateAndClean( inPolyData1 ) )
    {
        return EXIT_FAILURE ;
    }
    if( TriangulateAndClean( inPolyData2 ) )
    {
        return EXIT_FAILURE ;
    }
    std::string outputFieldSuffix ;
    if( targetInFields )
    {
      outputFieldSuffix = "_to_" + vtksys::SystemTools::GetFilenameWithoutExtension( vtkFile2 ) ;
    }
    vtkSmartPointer< vtkPolyData > outPolyData ;

    bool signedDistance = false ;
    if( distanceType == "signed_closest_point" )
    {
        signedDistance = true ;
    }
    if( ClosestPointDistance( inPolyData1 , inPolyData2 , signedDistance , outPolyData , outputFieldSuffix ) )
    {
        return EXIT_FAILURE ;
    }

    return WriteVTK( vtkOutput , outPolyData ) ;
}
