//Copyright (C) 2015 <>< Charles Lohr, see LICENSE file for more info.
//
//This particular file may be licensed under the MIT/x11, New BSD or ColorChord Licenses.


is_ntsc_running = false;

function KickNTSC()
{
	if( !is_ntsc_running )
		NTSCDataTicker();
}

window.addEventListener("load", LoadNTSC, false);
lastencs = [];
ntscloaded = false;

function LoadNTSC()
{
	for( var i = 0; i < 5; i++ )
	{
		CBs = "";
		for( var k = 0; k < 32; k++ )
		{
			CBs += "<INPUT TYPE=checkbox ID=ntcb" + i + "_" + k + ">";
		}
		$( "#CB" + i ).html( CBs );
	}
	KickNTSC();
}

function GotNTSC(req,data)
{
	var vs = data.split( ":" );
	for( var i = 0; i < 5; i++ )
	{
		num = parseInt( vs[i+1], 16 );
		if( lastencs[i] != num )
		{
			lastencs[i] = num;
			console.log( num );
			for( var k = 0; k < 32; k++ )
			{
				var ch = Math.pow( 2, 31-k ) & num;
				$("#ntcb"+i+"_"+k).prop('checked', ch?true:false);
			}
		}
	}
	ntscloaded = true;
	QueueOperation( "CD",  GotNTSCDefault );
} 

function GotNTSCDefault(req,data)
{
	var vs = data.split( ":" );
	for( var i = 0; i < 5; i++ )
	{
		num = parseInt( vs[i+1], 16 );
		var scmd = "0x";
		for( var k = 0; k < 4; k++ )
		{
			//console.log( (userchecks >> (24 - k*3))&0xff );
			scmd += tohex8( (num / Math.pow( 2, (24 - k*8)))  & 0xFF );
		}

		$("#d_"+i).prop( "value", scmd );
		$("#d_"+i).prop('disabled',true);
	}
	NTSCDataTicker();
} 


function NTSCDataTicker()
{
	if( IsTabOpen('NTSC') )
	{
		QueueOperation( "CG",  GotNTSC );

		is_ntsc_running = true;
		var pushchanges = false;
		var scmd = "CS:";

		for( i = 0; i < 5; i++ )
		{
			var userchecks = 0;
			for( var k = 0; k < 32; k++ )
			{
				var ch = Math.pow( 2,(31-k));
				userchecks += ($("#ntcb"+i+"_"+k).prop('checked'))?ch:0;
			}
			if( userchecks != lastencs[i] )
			{
				pushchanges = true;
			}

			var kcmd = "0x";
			for( var k = 0; k < 4; k++ )
			{
				kcmd += tohex8( (userchecks / Math.pow( 2, (24 - k*8)))  & 0xFF );
			}
			$("#c_"+i).prop( "value", kcmd );


			for( var k = 0; k < 4; k++ )
			{
				//console.log( (userchecks >> (24 - k*3))&0xff );
				scmd += tohex8( (userchecks / Math.pow( 2, (24 - k*8)))  & 0xFF );
			}
		}

		if( pushchanges && ntscloaded )
		{
			console.log( scmd );
			QueueOperation( scmd,  NTSCDataTicker );
		}
	}
	else
	{
		is_ntsc_running = 0;
	}
	//$( "#LEDPauseButton" ).css( "background-color", (is_leds_running&&!pause_led)?"green":"red" );

}




