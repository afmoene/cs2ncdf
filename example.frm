// $Id$
// Example csi2ncdf format file
title="MetAir (WU) eddycorrelation data " 
history="none"
id = 200 timevar = "time"  type="double"
// Write timevar "time" as a variable in decimal hours (doy not included)
id = 100 col_num = 2 var_name="doy" units="days since 2000-01-01" long_name="day of year" type="short" follow_id=200 missing_value=-9999
id = 100 col_num = 3 var_name="hour_min" units="-" type="short" time_mult=1.0 time_csi_hm=1  follow_id=200 missing_value=0
id = 200 col_num = 2 var_name="sec" units="-" time_mult=2.777e-4
id = 200 col_num = 3 ncol=3 dim_name="comp" var_name="velocity" units="mV" long_name="windspeed at lower level" 
id = 200 col_num = 8 var_name="Tsonic_lo" units="mV" scale_factor=0.05
id = 200 col_num = 9 var_name="Krypton_lo" units="mV" 
