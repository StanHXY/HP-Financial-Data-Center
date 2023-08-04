delete from T_INTERCFG;

/* Get all Company information parameters. */
insert into T_INTERCFG(typeid, intername, intercname, selectsql, colstr, bindin, keyid) 
values ('0101', 'getzhobtcode', 'National Station Parameters', 'select obtid, cityname, provname, lat, lon, height from T_ZHOBTCODE', 'obtid, cityname, provname, lat, lon, height', null, SEQ_INTERCFG.nextval);

/* Get Company information minute observation data by station. */
insert into T_INTERCFG(typeid, intername, intercname, selectsql, colstr, bindin, keyid) 
values ('0102', 'getzhobtmind1', 'National Station Minute Observation Data (By Station)', 'select obtid, to_char(ddatetime, ''yyyymmddhh24miss''), t, p, u, wd, wf, r, vis from T_ZHOBTMIND where obtid=:1', 'obtid, ddatetime, t, p, u, wd, wf, r, vis', 'obtid', SEQ_INTERCFG.nextval);

/* Get Company information minute observation data by time period. */
insert into T_INTERCFG(typeid, intername, intercname, selectsql, colstr, bindin, keyid) 
values ('0102', 'getzhobtmind2', 'National Station Minute Observation Data (By Time Period)', 'select obtid, to_char(ddatetime, ''yyyymmddhh24miss''), t, p, u, wd, wf, r, vis from T_ZHOBTMIND where ddatetime>=to_date(:1, ''yyyymmddhh24miss'') and ddatetime<=to_date(:2, ''yyyymmddhh24miss'')', 'obtid, ddatetime, t, p, u, wd, wf, r, vis', 'begintime, endtime', SEQ_INTERCFG.nextval);

/* Get Company information minute observation data by station and time period. */
insert into T_INTERCFG(typeid, intername, intercname, selectsql, colstr, bindin, keyid) 
values ('0102', 'getzhobtmind3', 'National Station Minute Observation Data (By Station and Time Period)', 'select obtid, to_char(ddatetime, ''yyyymmddhh24miss''), t, p, u, wd, wf, r, vis from T_ZHOBTMIND where obtid=:1 and ddatetime>=to_date(:2, ''yyyymmddhh24miss'') and ddatetime<=to_date(:3, ''yyyymmddhh24miss'')', 'obtid, ddatetime, t, p, u, wd, wf, r, vis', 'obtid, begintime, endtime', SEQ_INTERCFG.nextval);

exit;
