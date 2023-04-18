/*
 * Copyright JaguarDB
 *
 * This file is part of JaguarDB.
 *
 * JaguarDB is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * JaguarDB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with JaguarDB (LICENSE.txt). If not, see <http://www.gnu.org/licenses/>.
 */
#include <JagGlobalDef.h>
#include <sys/time.h>
#include <JagRange.h>
#include <JagUtil.h>
#include <JagTime.h>

// sp1: "OJAG=6=test.tr.b=RG=T 0:0 1670703625123456 1733862025123456"
// sp2: "OJAG=6=test.tr.b=RG=T 0:0 1670703625123456 1733862025123456"
// sp2: "CJAG=0=0=RG=d 0:0:0:0:0:0 2022-11-11_10:10:10|2024-10-01_03:02:03"

bool 
JagRange::doRangeWithin( const JagParseAttribute &jpa, const Jstr &mk1, const Jstr &colType1, 
						 int srid1, const JagStrSplit &sp1, 
						 const Jstr &mk2, const Jstr &colType2, int srid2, const JagStrSplit &sp2, 
						 bool strict )
{
    dn("s029283004 doRangeWithin colType1=%s  colType2=%s", colType1.s(), colType2.s() );
	if (  colType2 != JAG_C_COL_TYPE_RANGE ) {
        dn("s828393004 return false");
		return false;
	} 

    /**
    dn("s111129 sp1:");
    sp1.print();
    dn("s111129 sp2:");
    sp2.print();
    **/

    Jstr begin2, end2;
    int  rc2 = getBeginEnd(jpa, sp2, colType2, begin2, end2);
    if  ( rc2 < 0 ) {
        dn("s8297004 return false rc2=%d", rc2);
		return false;
    }

    Jstr subtype = getSubtype( mk1, sp1, mk2, sp2 );

	if ( colType1 == JAG_C_COL_TYPE_RANGE ) {
        Jstr begin1, end1;
        int  rc1 = getBeginEnd(jpa, sp1, colType1, begin1, end1);
        if  ( rc1 < 0 ) {
            dn("s827004 return false rc1=%d", rc1);
		    return false;
        }

        dn("s8840023 rangeWithinRange begin1=[%s] end1=[%s] ...", begin1.s(), end1.s() );
        dn("s8840023 rangeWithinRange begin2=[%s] end2=[%s] ...", begin2.s(), end2.s() );

		return rangeWithinRange( subtype, begin1, end1, begin2, end2, strict );
	} else {

		Jstr data;
		if ( sp1.length() >= 3 ) {
			data = sp1[2];
		} else {
			data = sp1[0];
		}

        dn("s0230288 data=[%s]", data.s() );
        if ( isDateTime( subtype ) ) {
            if ( JagTime::isDateOrTimeFormat( data ) ) {
                data.replace("_T", ' ');
            } else if ( subtype == JAG_C_COL_TYPE_TIMEMICRO ) {
                Jstr nd;
                JagTime::convertTimeToStr( data, nd, JAG_TIME_SECOND_MICRO );
                data = nd;
                dn("s253038 convertTimeToStr JAG_C_COL_TYPE_TIMEMICRO new data=[%s]", data.s() );
            } else if ( subtype == JAG_C_COL_TYPE_TIMENANO ) {
                Jstr nd;
                JagTime::convertTimeToStr( data, nd, JAG_TIME_SECOND_NANO );
                data = nd;
                dn("s253038 convertTimeToStr JAG_C_COL_TYPE_TIMENANO new data=[%s]", data.s() );
            } else if ( subtype == JAG_C_COL_TYPE_DATETIMEMICRO ) {
                Jstr nd;
                JagTime::convertDateTimeToStr( data, nd, false, JAG_TIME_SECOND_MICRO );
                data = nd;
            } else if ( subtype == JAG_C_COL_TYPE_DATETIMENANO ) {
                Jstr nd;
                JagTime::convertDateTimeToStr( data, nd, false, JAG_TIME_SECOND_NANO );
                data = nd;
            } else if ( subtype == JAG_C_COL_TYPE_DATETIMEMILLI ) {
                Jstr nd;
                JagTime::convertDateTimeToStr( data, nd, false, JAG_TIME_SECOND_MILLI );
                data = nd;
            } else if ( subtype == JAG_C_COL_TYPE_DATETIMESEC ) {
                Jstr nd;
                JagTime::convertDateTimeToStr( data, nd, false, JAG_TIME_SECOND);
                data = nd;
            } else {
                data.replace("_T", ' ');
            }
        }

        dn("s02129208 data=[%s]", data.s() );

		return pointWithinRange( subtype, data, begin2, end2, strict );
	}
}

// range1 [begin1,... end1],  range2 [begin2, ......   end2]
// true if range1 is within range2
bool JagRange::rangeWithinRange( const Jstr &subtype, 
								 const Jstr &begin1, const Jstr &end1,
								 const Jstr &begin2, const Jstr &end2, bool strict )
{
    dn("s3333001 rangeWithinRange begin1=[%s] end1=[%s]", begin1.s(), end1.s() );
    dn("s3333001 rangeWithinRange begin2=[%s] end2=[%s]", begin2.s(), end2.s() );
    dn("s22230 subtype=[%s]", subtype.s() );

    if ( isDateTime( subtype ) ) {
		if ( strict ) {
			if ( strcmp( begin2.c_str(), begin1.c_str() ) < 0 && strcmp( end1.c_str(), end2.c_str() ) < 0 ) {
                dn("s4500838 strict true");
				return true;
			}
		} else {
			if ( strcmp( begin2.c_str(), begin1.c_str() ) <= 0 && strcmp( end1.c_str(), end2.c_str() ) <= 0  ) {
                dn("s4500838 non-strict true");
				return true;
			}
		}
    } else {
		double begin1n = jagatof( begin1 );
		double end1n = jagatof( end1 );
		double begin2n = jagatof( begin2 );
		double end2n = jagatof( end2 );
		if ( strict ) {
			if ( begin2n < begin1n && end1n < end2n ) {
                dn("s45838 strict true");
				return true;
			}
		} else {
			if ( jagLE( begin2n, begin1n ) && jagLE( end2n, end1n ) ) {
                dn("s45838 non-strict true");
				return true;
			}
		}
	}
	
	return false;
}


bool JagRange::pointWithinRange( const Jstr &subtype, const Jstr &data, 
								 const Jstr &begin2, const Jstr &end2, bool strict )
{
    dn("s3433001 pointWithinRange data=[%s]", data.s() );
    dn("s3433001 pointWithinRange begin2=[%s] end2=[%s]", begin2.s(), end2.s() );
    dn("s24230 subtype=[%s]", subtype.s() );

    if ( isDateTime( subtype ) ) {
        dn("s933939 isDateTime");
		if ( strict ) {
			if ( strcmp( begin2.c_str(), data.c_str() ) < 0 && strcmp( data.c_str(), end2.c_str() ) < 0 ) {
				return true;
			}
		} else {
			if ( strcmp( begin2.c_str(), data.c_str() ) <= 0 && strcmp( data.c_str(), end2.c_str() ) <= 0  ) {
				return true;
			}
		}
    } else {
		double datan = jagatof( data );
		double begin2n = jagatof( begin2 );
		double end2n = jagatof( end2 );
		if ( strict ) {
			if ( begin2n < datan && datan < end2n ) {
				return true;
			}
		} else {
			if ( jagLE( begin2n, datan ) && jagLE( end2n, datan ) ) {
				return true;
			}
		}
	}
	
	return false;
}

bool 
JagRange::doRangeIntersect( const JagParseAttribute &jpa, const Jstr &mk1, const Jstr &colType1, 
						    int srid1, const JagStrSplit &sp1, 
						    const Jstr &mk2, const Jstr &colType2, int srid2, const JagStrSplit &sp2 )
{
    dn("s029283004 doRangeIntersect");
	if (  colType2 != JAG_C_COL_TYPE_RANGE ) {
        dn("s828393004 return false");
		return false;
	} 

    /**
    dn("s111129 sp1:");
    sp1.print();
    dn("s111129 sp2:");
    sp2.print();
    **/

    Jstr begin2, end2;
    int  rc2 = getBeginEnd(jpa, sp2, colType2, begin2, end2);
    if  ( rc2 < 0 ) {
        dn("s8297004 return false rc2=%d", rc2);
		return false;
    }

    Jstr subtype = getSubtype( mk1, sp1, mk2, sp2 );

	if ( colType1 == JAG_C_COL_TYPE_RANGE ) {
        Jstr begin1, end1;
        int  rc1 = getBeginEnd(jpa, sp1, colType1, begin1, end1);
        if  ( rc1 < 0 ) {
            dn("s827004 return false rc1=%d", rc1);
		    return false;
        }

        dn("s8840023 rangeIntersectRange begin1=[%s] end1=[%s] ...", begin1.s(), end1.s() );
        dn("s8840023 rangeIntersectRange begin2=[%s] end2=[%s] ...", begin2.s(), end2.s() );

		return rangeIntersectRange( subtype, begin1, end1, begin2, end2 );
	} else {

		Jstr data;
		if ( sp1.length() >= 3 ) {
			data = sp1[2];
		} else {
			data = sp1[0];
		}

        dn("s02929288 data=[%s]", data.s() );
        if ( isDateTime( subtype ) ) {
            if ( JagTime::isDateOrTimeFormat( data ) ) {
                data.replace("_T", ' ');
            } else if ( subtype == JAG_C_COL_TYPE_TIMEMICRO ) {
                Jstr nd;
                JagTime::convertTimeToStr( data, nd, JAG_TIME_SECOND_MICRO );
                data = nd;
                dn("s233038 convertTimeToStr JAG_C_COL_TYPE_TIMEMICRO new data=[%s]", data.s() );
            } else if ( subtype == JAG_C_COL_TYPE_TIMENANO ) {
                Jstr nd;
                JagTime::convertTimeToStr( data, nd, JAG_TIME_SECOND_NANO );
                data = nd;
                dn("s233038 convertTimeToStr JAG_C_COL_TYPE_TIMENANO new data=[%s]", data.s() );
            } else if ( subtype == JAG_C_COL_TYPE_DATETIMEMICRO ) {
                Jstr nd;
                JagTime::convertDateTimeToStr( data, nd, false, JAG_TIME_SECOND_MICRO );
                data = nd;
            } else if ( subtype == JAG_C_COL_TYPE_DATETIMENANO ) {
                Jstr nd;
                JagTime::convertDateTimeToStr( data, nd, false, JAG_TIME_SECOND_NANO );
                data = nd;
            } else if ( subtype == JAG_C_COL_TYPE_DATETIMEMILLI ) {
                Jstr nd;
                JagTime::convertDateTimeToStr( data, nd, false, JAG_TIME_SECOND_MILLI );
                data = nd;
            } else if ( subtype == JAG_C_COL_TYPE_DATETIMESEC ) {
                Jstr nd;
                JagTime::convertDateTimeToStr( data, nd, false, JAG_TIME_SECOND);
                data = nd;
            } else {
                data.replace("_T", ' ');
            }
        }

		return pointWithinRange( subtype, data, begin2, end2, false );
	}
}


bool JagRange::rangeIntersectRange( const Jstr &subtype, 
									const Jstr &begin1, const Jstr &end1,
								 	const Jstr &begin2, const Jstr &end2 )
{
    if ( isDateTime( subtype ) ) {

		if ( strcmp( end1.c_str(), begin2.c_str() ) <= 0 ) {
            // no-overlap OK
		} else if (  strcmp( end2.c_str(), begin1.c_str() ) <= 0  ) {
            // no-overlap OK
        } else {
            // there is some strict overlap -- time conflict
            return true;
        }
    } else {
		double begin1n = jagatof( begin1 );
		double end1n = jagatof( end1 );
		double begin2n = jagatof( begin2 );
		double end2n = jagatof( end2 );

        if ( jagLE( end1n, begin2n ) ) {
        } else if ( jagLE(end2n, begin1n) ) {
        } else {
            return true;
        }
	}
	
	return false;
}

bool 
JagRange::doRangeSame( const JagParseAttribute &jpa, const Jstr &mk1, const Jstr &colType1, 
						 int srid1, const JagStrSplit &sp1, 
						 const Jstr &mk2, const Jstr &colType2, int srid2, const JagStrSplit &sp2 )
{
    dn("s0293004 doRangeSame");
	if (  colType2 != JAG_C_COL_TYPE_RANGE ) {
        dn("s828393004 return false");
		return false;
	} 

    /**
    dn("s111129 sp1:");
    sp1.print();
    dn("s111129 sp2:");
    sp2.print();
    **/

    Jstr begin2, end2;
    int  rc2 = getBeginEnd(jpa, sp2, colType2, begin2, end2);
    if  ( rc2 < 0 ) {
        dn("s8297004 return false rc2=%d", rc2);
		return false;
    }

    Jstr begin1, end1;
    int  rc1 = getBeginEnd(jpa, sp1, colType1, begin1, end1);
    if  ( rc1 < 0 ) {
        dn("s827004 return false rc1=%d", rc1);
		    return false;
    }

    Jstr subtype = getSubtype( mk1, sp1, mk2, sp2 );

    dn("s880023 begin1=[%s] end1=[%s] ...", begin1.s(), end1.s() );
    dn("s880023 begin2=[%s] end2=[%s] ...", begin2.s(), end2.s() );

	return rangeSameRange( subtype, begin1, end1, begin2, end2 );
}

bool JagRange::rangeSameRange( const Jstr &subtype, 
							   const Jstr &begin1, const Jstr &end1,
							   const Jstr &begin2, const Jstr &end2 )
{
    if ( isDateTime( subtype ) ) {
		if ( begin2 == begin1 && end1 == end2 ) {
            return true;
        }
    } else {
		double begin1n = jagatof( begin1.c_str() );
		double end1n = jagatof( end1.c_str() );
		double begin2n = jagatof( begin2.c_str() );
		double end2n = jagatof( end2.c_str() );
		if ( jagEQ(begin2n,begin1n) && jagEQ(end1n,end2n) ) return true;
	}
	
	return false;
}

// sp2: "OJAG=6=test.tr.b=RG=T 0:0 1670703625123456 1733862025123488"
// sp2: "CJAG=0=0=RG=d 0:0:0:0:0:0 2022-11-11_10:10:10|2024-10-01_03:02:03"
// < 0 for error;  0 OK
// begin:  "yyyy-mm-dd hh:mm:ss.ccc" or "hh:mm:ss.ccccc" or "yyyy-mm-dd"
// begin:  "1670703625123456"   end: "1733862025123488"
int JagRange::getBeginEnd(const JagParseAttribute &jpa, const JagStrSplit &sp, const Jstr &colType, Jstr &begin, Jstr &end)
{
    if (sp.length() < 3 ) return -1;

    if ( sp.length() == 3 ) {
        Jstr arg3 = sp[2];
        if ( ! arg3.containsChar('|') ) {
            return -5;
        }

        JagStrSplit ap( arg3, '|');
        begin = ap[0];
        end = ap[1];

	    begin.replace("_T", ' ' );
	    end.replace("_T", ' ' );

        return 0;
    }

    if ( sp.length() == 4 ) {
        begin = sp[2];
        end = sp[3];

        /***
        JagStrSplit hp( sp[0], 5, '=');
        if ( hp[0] != JAG_OJAG ) {
            dn("s222208 error not OJAG header");
            abort();
        }
        ***/

        //Jstr  subType = hp[4];
        Jstr  subType = JagRange::getLastType( sp[0] );

        if ( isDateTime( subType ) ) {
            Jstr    outstr;
            JagTime::getStrFromEpochTime( jpa, begin.toULong(), subType, outstr );
            begin = outstr;

            JagTime::getStrFromEpochTime( jpa, end.toULong(), subType, outstr );
            end = outstr;
        }

        return 0;
    }

    return -10;
}

Jstr JagRange::getSubtype( const Jstr &mk1, const JagStrSplit &sp1, const Jstr &mk2, const JagStrSplit &sp2 )
{
    dn("s2223939 getSubtype mk1=%s  mk2=%s", mk1.s(), mk2.s() );

    Jstr subtype;

    if ( mk1 == JAG_OJAG ) {
        subtype = getLastType( sp1[0] ); 
    } else if ( mk2 == JAG_OJAG ) {
        subtype = getLastType( sp2[0] ); 
    } else {
        Jstr tp1 =  getLastType( sp1[0] );
        Jstr tp2 =  getLastType( sp2[0] );

        if ( tp1 != JAG_C_COL_TYPE_DT_NONE ) {
            subtype = tp1;
        } else {
            subtype = tp2;
        }
    }

    dn("r202396 return subtype=[%s]", subtype.s() );
    return subtype;
}

Jstr JagRange::getLastType( const Jstr &hdr )
{
    Jstr type;
    int n = strchrnum(hdr.s(), '=');
    if ( 4 == n ) {
        JagStrSplit sp(hdr, 5, '=');
        type = sp[4];
    } else if ( 3 == n ) {
        JagStrSplit sp(hdr, 4, '=');
        type = sp[3];
    }

    return type;
}


