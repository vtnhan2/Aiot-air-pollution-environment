$ErrorActionPreference = "Stop"
try {
    $word = New-Object -ComObject Word.Application
    $word.Visible = $false
    $word.DisplayAlerts = 0 # wdAlertsNone

    $coverPath = "d:\Work\Outsource_AioT_environment\Bia mau.docx"
    $reportPath = "d:\Work\Outsource_AioT_environment\report.docx"
    $outputPath = "d:\Work\Outsource_AioT_environment\report_final.docx"
    
    # Open ReadOnly, AddToRecentFiles=$false, Visible=$false
    $docCover = $word.Documents.Open($coverPath, $false, $false, $false)
    $docReport = $word.Documents.Open($reportPath, $false, $true, $false)
    
    # Select all report content and copy
    $docReport.ActiveWindow.Selection.WholeStory()
    $docReport.ActiveWindow.Selection.Copy()
    
    # Activate cover document
    $docCover.Activate()
    # Go to end of cover
    $word.Selection.EndKey(6) # wdStory
    # Insert page break
    $word.Selection.InsertBreak(7) # wdPageBreak
    # Paste
    $word.Selection.Paste()
    
    # Save as new file
    $docCover.SaveAs([ref]$outputPath)
    
    $docReport.Close(0) # wdDoNotSaveChanges
    $docCover.Close(0)
    $word.Quit()
    
    Write-Output "Merged successfully into report_final.docx"
} catch {
    Write-Error "Error: $_"
    if ($word) { $word.Quit() }
}
