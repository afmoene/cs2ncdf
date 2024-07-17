# Intro

The purpose of **csi2ncdf** is to convert the data files that are produced by dataloggers of Campbell Scientific (CSI, see www.campbellsci.com (http://www.campbellsci.com)) to NetCDF files (see www.unidata.ucar.edu/packages/netcdf (http://www.unidata.ucar.edu/packages/netcdf/)). This refers both to the classic binary format, ASCII format and there is some support for TOB  files (TOB1-TOB3) and TOA5 files. The conversion of ASCII files to NetCDF is dealt with in more detail in section 'ASCII to NetCDF'.

In addition to pure conversion, the program can also be used to make selections of the data, while converting from CSI to NetCDF. Furthermore, the contents of the CSI file can be dumped to standard output (screen) for quick inspection, or to convert the CSI file to ASCII (in this way it is a very simplified version of the Campbell 'split' program).  The program is invoked as:  

    csi2ncdf [-i infile [-i infile] -o outfile -f formatfile] [-l num_lines] 
               [-c "condition" ....]
               [-b "start condition" ]
               [-e "stop condition"]
               [-t txttype]
               [-n newtype]
               [-k colnum] [-k colnum]
               [-a]
               -s [-h]

where the square brackets denote optional switches: valid commandlines are for example:

    csi2ncdf -i csi.dat -o foo.nc -f format.con
    csi2ncdf -i csi1.dat -i csi2.dat -o foo.nc -f format.con
    csi2ncdf -i csi.dat -l 10
    csi2ncdf -i csi.dat -o foo.nc -f format.nc -c "A100 C2 > 1400"
    csi2ncdf -i csi.dat -o foo.nc -f format.con -s
    csi2ncdf -i csi.dat -o foo.nc -f format.con -b "A100 c2 => 1400" -e "A100 c2 ==1500"
    csi2ncdf -i csi.dat -o foo.nc -f format.con -t csv -a
    csi2ncdf -i tob1.dat -n tob1 -l -1 -k 2 -k 3 
    csi2ncdf -h

The flags have the following meaning: 

| flag and argument | description |
| ----------------- | ----------- |
| -i infile         | name of input (Campbell) file; instead of a file name you can specify a dash (-) which will result in reading from standard input, rather than a a file. This option is useful for using csi2ncdf behind a pipe (\|). More than one -i switch may be used. Files are handled in the order that they have been specified. Specifying more than one input file enables one to concatenate data from various files into one NetCDF file. |
| -o outfile | name of output (NetCDF) file | 
| -f formatfile | name of file that describes the format of the  Campbell file |
| -l num_lines | list num_lines lines from input file to screen if num_lines   equals -1, all lines are listed  (thus the program can be used as a simple 'split' progam) |
| -c condition | only output data when certain conditions are met (see the Section called Conditions on Conditions for a description) |
| -b condition | start output when condition is met |
| -e condition | stop output when condition is met |
| -t txttype |input file is a text file, where txttype specifies the separator: csv (comma separated ), ssv (space separated), tsv (tab separated). If the file has no column with an ArrayID, one should specifiy the -a  flag |
| -a | the input (text) file has no ArrayID. Take a fake value from the first definition in the format file (i.e. all definitions in the format file should have the same ArrayID). If data are listed to standard output (and no format file is present), the arrayID is set internally to 0 (zero). In this way, conditions can still be used.|
| -s | be sloppy about errors in input file:  give warning but not abort |
| -n newtype | input file is of new, table oriented format: tob1 (table oriented binary type 1: only output to standard output), tob2 (table oriented binary type 1: only output to standard output), tob3 (table oriented binary type 1: only output to standard output), toa5  (a table origented text file: only output to standard output)| 
| -k colnum | when writing to standard output, this switch can be used to select the columns to be written. More than one -k option is allowed. If not -k option used, all columns are written.|
| -x skip_lines | Skip skip_lines when reading a text file (to skip a header for instance) |
| -y | convert the -optional- time information in a TOB1 file (seconds since 1990, nanoseconds and record counter) to readable time information (YYYY DDD HHMM SS.SSSS) |
| -d decimal_places | Set number of decimal places in text output to standard output (either for listing file, or for conversion of TOB and TOA formats)|
| -h | show help on screen |

More detailed information can be found in the readme file in the doc folder.
