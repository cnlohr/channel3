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
var programs = [];

var last_custom_program_data;
var last_custom_program_color;
//Utility functions for the inner scripts:

var DefaultCode = "autoset = 0;\ncolor = 5;\nvar freq1 = 61.25;\nvar power1 = 1.0;\nvar phase = 0.1;\nvar freq2 = freq1 + 315.0/88.0;\nvar power2 = 1.0;\nfor( var i = 0; i < nrsamps; i++ ) {\n  \
var ev = Math.sin(i*6.283185 * freq1 / spsout )*power1;\n  \
ev += Math.sin(i*6.283185 * freq2 / spsout + phase*6.283185 )*power2;\n  \
sampout[i] = ev;\n  \
sampout[i] = (sampout[i]>0.5)?1:-1;\n\
}\n\
kmsg='Code Running.'\n\
";

var CurrentCode = DefaultCode;

function MakeProgram( KexecCode )
{
	return "function Kexec() { " + KexecCode + " };\n\
	var i = 0;\n\
	var spsout = 80;\n\
	var dftlow = 0.0;\n\
	var color = -1;\n\
	var autoset = 0;\n\
	var dfthigh = 0.0;\n\
	var nrsamps = 1408;\n\
	var cansend = false;\n\
	var dftwindow = 50;\n\
	var sampout = [];\n\
	var kmsg = '';\n\
	\n\
	function timedCount() {\n\
	\n\
		var k = {};\n\
		k.color = color;\n\
		k.rangel = dftlow;\n\
		k.rangeh = dfthigh;\n\
		k.msg = '';\n\
		k.sampout = [];\n\
		k.dft = [];\n\
		k.autoset = autoset\n\
		k.dftmax = -1.0\n\
		if( cansend ) { \n\
			try {\n\
				Kexec(); \n\
				k.msg = kmsg;\n\
				k.sampout = sampout;\n\
				k.dftsamples = 600;\n\
				var dftmax = 0.0;\n\
				if( nrsamps-dftwindow < 0 ) throw 'DFT Window invalid.';\n\
				if( dftwindow < 1 ) throw 'DFT Window too small.';\n\
				var dftstart = Math.floor(Math.random()*(nrsamps-dftwindow));\n\
				for( i = 0; i < k.dftsamples; i++ )\n\
				{\n\
					var irel = i / (k.dftsamples-1);\n\
					var frel = (dfthigh - dftlow) * irel + dftlow;\n\
					var power = 0.0;\n\
					var preal = 0.0;\n\
					var pimag = 0.0;\n\
					for( var j = 0; j < dftwindow; j++ )\n\
					{\n\
						var envelope = Math.sin( j/dftwindow * 3.14159 );\n\
						var t = j*6.283185*frel / spsout;\n\
						preal += Math.sin( t ) * sampout[j+dftstart] * envelope;\n\
						pimag += Math.cos( t ) * sampout[j+dftstart] * envelope;\n\
					}\n\
					power += Math.sqrt( preal * preal + pimag * pimag );\n\
					power /= sampout.length;\n\
					k.dft[i] = power;\n\
					if( power > dftmax ) dftmax = power;\n\
				}\n\
				k.dftmax = dftmax\n\
			} catch(err) { k.msg = err; } \n\
			postMessage(k);\n\
			cansend = false;\n\
		}\n\
		setTimeout('timedCount()',20);\n\
	}\n\
	self.addEventListener('message', function(e) { \n\
	  dftlow = e.data[0]; dfthigh = e.data[1];  dftwindow = e.data[2]; spsout = e.data[3]; cansend = true; \n\
	}, false);\n\
	\n\
	timedCount();\n\
	";
}
var ntscWorker;  //A webworker.
var NTSCCanvas;

function UpdateDFTRange()
{
	var low = Number( $("#FTLow").val() );
	var high = Number( $("#FTHigh").val() );
	var spsout = Number( $("#FTSamplerate").val() );
	var window = Number( $("#FTWindow").val() );
	if( typeof( ntscWorker ) != 'undefined' )
	    ntscWorker.postMessage( [low, high, window, spsout] ); // Start the worker.
}

function NTSCGotMessage(e) {
	//Expecting an object in the format:
	//  .color = n
	//  .data = [array of time domain values]
	//  .dft = [array of up freq values]
	//  .rangel = 60.0
	//  .rangeh = 66.6
	var rangebot = 16;
	var rangeleft = 0;
	var dbscale = $("#usedbscale").prop('checked');
	var w = NTSCCanvas.width-1-rangeleft;
	var h = NTSCCanvas.height-1;
	var rl = e.data.rangel;
	var rh = e.data.rangeh;
	var dft = e.data.dft;
	var dftmax = e.data.dftmax;
	var ctx=NTSCCanvas.getContext("2d");
	ctx.clearRect(0, 0, w+rangeleft+1, h);
	ctx.font = "48px Arial"
	ctx.textBaseline="bottom"; 
	ctx.textAlign = "right";
	ctx.fillStyle="#ff9090";
	ctx.fillText( "Simulation", w, 50 );
	ctx.fillStyle="#000000";



	ctx.strokeStyle="#000000";
	ctx.font = "18px Arial";

	$("#UploadColor").prop("disabled", true );

	ctx.textBaseline="bottom"; 
	ctx.textAlign = "left";

	ctx.fillText( e.data.msg, 0, 40 );

	ctx.textBaseline="top"; 
	ctx.fillText( dftmax.toPrecision(4), 0, 0 );

	ctx.beginPath();
	ctx.strokeStyle="#000000";
	ctx.moveTo(0+rangeleft,h-rangebot);
	ctx.lineTo(w+rangeleft,h-rangebot);
	ctx.moveTo(0+rangeleft,0);
	ctx.lineTo(w+rangeleft,0);
	ctx.stroke();

	ctx.strokeStyle="#FF0000";

	var bars = 10;
	for( var bar = 0; bar <= bars; bar++ )
	{
		var bp = bar / bars;
		var pos = bp * w;
		var f = (rh - rl) * bp + rl;

		if( bar == bars )
			ctx.textAlign = "right";
		else if( bar == 0 )
			ctx.textAlign = "left";
		else
			ctx.textAlign = "center";
		ctx.textBaseline="top"; 
		ctx.fillText( f, pos+rangeleft, h-rangebot+1 );

		ctx.beginPath();
		ctx.strokeStyle="#808080";
		ctx.setLineDash([2,1]);
		ctx.moveTo(pos+0.5+rangeleft,0);
		ctx.lineTo(pos+0.5+rangeleft,h-rangebot);
		ctx.stroke();
	}

	ctx.textAlign = "left";
	ctx.textBaseline="top"; 

	//Draw the data.
	var dftp = 0;

	ctx.strokeStyle="#404000";
	ctx.setLineDash([0]);
	ctx.lineWidth = 2;
	ctx.beginPath();

	for( dftp = 0; dftp < dft.length; dftp++ )
	{
		var bp = dftp / dft.length;
		var pos = bp * w;
		var val = dft[dftp]/dftmax;
		if( dbscale ) val = (Math.log( dft[dftp] )+8)/8.0;
		var y = (1.0-(val)) * (h-rangebot);
		if( y > (h-rangebot) ) y = (h-rangebot);
		if( dftp == 0 )
			ctx.moveTo(0+rangeleft,y);
		else
			ctx.lineTo(pos+0.5+rangeleft,y);		
	}
	ctx.stroke();
	ctx.lineWidth = 1;
	ctx.strokeStyle="#ff0000";
	ctx.setLineDash([0]);

	if( e.data.autoset )
	{
		UploadColorFunc( null );
	}

	last_custom_program_data = e.data.sampout;
	last_custom_program_color =  e.data.color;
	//Fix 'Upload' button.
	$("#UploadColor").val( "Upload Col " + e.data.color );
	$("#UploadColor").prop("disabled", false );
	UpdateDFTRange();
}

function CVSSetResp( e )
{
	//Response from setting CV.  We should probably check to make sure the operation didn't fail.
}

function UploadColorFunc( e )
{
	console.log( "Uploading " + last_custom_program_color );

	//last_custom_program_data
 
	if( last_custom_program_color >= 0 )
	{
		var CVSet = "CV " + tohex8( last_custom_program_color );
		var lwords = last_custom_program_data.length/8;
		var i;
		var place = 0;

		for( i = 0; i < lwords; i++ )
		{
			var lvword = 0;
			for( var k = 7; k >= 0; k-- )
			{
				var p = last_custom_program_data[place++];
				lvword |= ( p > 0 )?(1<<k):0;
			}
			CVSet += tohex8( lvword );
		}

		QueueOperation( CVSet, CVSSetResp );
	}
}


function ReworkWorker( e )
{
	//console.log( "rework" + $("#NTSCprogram").val() );
	var code = $("#NTSCprogram").val();
	if( CurrentCode != code )
	{
		CurrentCode = code;
		HandleNewWebWorkerCode();
	}
}

function WWError( e )
{
	k = {};
	k.data = {};
	console.log( e );
	k.data.msg = 'ERROR: Line ' + e.lineno + ': ' + e.message;
	NTSCGotMessage( k );
}

function ChangeProgram( e )
{
	//console.log( "change"  );
	var ProgramID = $("#ProgramId").val();
	if( ProgramID == null ) ProgramID = 0;

	pgms = JSON.parse( localStorage.ntprograms );

	$("#NTSCprogram").val( pgms[ProgramID].dat );
	$("#NTSCSaveName").val( pgms[ProgramID].nam );
	ReworkWorker( $("#NTSCprogram") );
}

function HandleNewWebWorkerCode()
{
	var pgm = MakeProgram( CurrentCode );

	if( ntscWorker ) ntscWorker.terminate();
	blobURL = URL.createObjectURL( new Blob([pgm], { type: 'application/javascript' } ) );

	try
	{
		ntscWorker = new Worker(blobURL);
	    ntscWorker.addEventListener("error", WWError, false);
		ntscWorker.onmessage = NTSCGotMessage;
	} catch( e )
	{
		IssueSystemMessage( e.toString() );
		ntscWorker = null;
	}

	$("#UploadColor").prop("disabled", true );

	UpdateDFTRange(); //Actually posts a message saying it's okay!
} 

function UpdateOptionArray()
{
	var pgms = JSON.parse( localStorage.ntprograms );
	$('#ProgramId')
		.find('option')
		.remove()
		.end();

	var matching = 0;
	var matchname = $("#NTSCSaveName").val();
	for( var i = 0; i < pgms.length; i++ )
	{
		$('#ProgramId').append($("<option></option>")
	         .attr("value",i)
	         .text(pgms[i].nam));
		if( pgms[i].nam == matchname ) matching = i;
	}

	$('#ProgramId').val( matching );
}

function SaveNTSCProgram()
{

	var sn = $("#NTSCSaveName").val();
	var sp = $("#NTSCprogram").val();

	pgms = JSON.parse( localStorage.ntprograms );
	var i;
	for( i = 0; i < pgms.length; i++ )
	{
		if( pgms[i].nam == sn )
		{
			pgms[i].nam = sn;
			pgms[i].dat = sp;
			break;
		}
	}
	if( i == pgms.length )
	{
		newprog = {};
		newprog.nam = sn;
		newprog.dat = sp;
		pgms.push( newprog );
	}
	console.log( pgms );
	localStorage.ntprograms = JSON.stringify(pgms);

	UpdateOptionArray();
}

function LoadNTSC()
{
	NTSCCanvas = document.getElementById("FTCanvas");

	//	set localStorage.ntprograms = "" to force a flush.
	if( typeof( localStorage.ntprograms ) == "undefined" || localStorage.ntprograms == null || localStorage.ntprograms.length < 1 )
	{
		console.log( "No ntprograms.  Initializing." );
		programs = [];
		pgm = {};
		pgm.nam = "Col5Def";
		pgm.dat = DefaultCode;
		programs[0] = pgm;
		localStorage.ntprograms = JSON.stringify(programs);
		CurrentCode = DefaultCode;
		//console.log( programs );
	}

	//Enumerate the values into the option array.
	UpdateOptionArray();

	$("#FTSamplerate").val( "80.0" );
	$("#FTLow").val( "00.0" );
	$("#FTHigh").val( "40.0" );
	$("#FTWindow").val( "1408" );
	$("#FTSamplerate").change(UpdateDFTRange);
	$("#FTLow").change(UpdateDFTRange);
	$("#FTHigh").change(UpdateDFTRange);
	$("#FTWindow").change(UpdateDFTRange);
	UpdateDFTRange();

	$("#ProgramId").click(ChangeProgram);
	$("#ProgramId").on("change", ChangeProgram);
	ChangeProgram($("#ProgramId")); 

	$(document).delegate('#NTSCprogram','change keyup paste',ReworkWorker);

	HandleNewWebWorkerCode();

	KickNTSC();
}



function ntsc_set()
{
	CVSet = "CO ";
	CVSet += tohex8( Number($("#ScreenNumber").val()) );
	CVSet += tohex8( $("#EnableAdvance").prop('checked')?1:0 );
	CVSet += tohex8( Number($("#ScreenJamb").val()) );
	QueueOperation( CVSet, null );
}


function NTSCDataTicker()
{
	if( IsTabOpen('NTSC') )
	{
		//QueueOperation( "CG",  GotNTSC );

		is_ntsc_running = true;
	}
	else
	{
		is_ntsc_running = 0;
	}
	//$( "#LEDPauseButton" ).css( "background-color", (is_leds_running&&!pause_led)?"green":"red" );

}



