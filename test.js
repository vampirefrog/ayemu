
function AYPlayer() {
	var self = this;
	self.playing = false;
	self.ready = false;

	self.init_player = self.play_file = self.render_samples = null;
	self.bufSize = 4096;
	self.bufL = self.bufR = 0;
	self.audioCtx = null;
	self.scriptNode = null;

	self.queue = [];
	self.readyCallbacks = [];

	self.getPlayer = function(cb) {
		if(this.ready) {
			cb();
			return;
		} else {
			this.readyCallbacks.push(cb);

			// load emscripten Module
			if(typeof Module == 'undefined') {
				Module = {
					locateFile(f) {
						return f;
					},
					onRuntimeInitialized: function() {
						console.log('runtime initialized');
						self.init_player = Module.cwrap('init_player', null, ['number', 'number', 'number', 'number']);
						self.play_file = Module.cwrap('play_file', 'number', ['number', 'number', 'number']);
						self.get_num_songs = Module.cwrap('get_num_songs', 'number', []);
						self.get_song_name = Module.cwrap('get_song_name', 'string', ['number']);
						self.render_samples = Module.cwrap('render_samples', 'number', []);

						self.bufL = Module._malloc(self.bufSize * 4);
						self.bufR = Module._malloc(self.bufSize * 4);

						self.audioCtx = new AudioContext();
						self.scriptNode = self.audioCtx.createScriptProcessor(self.bufSize, 0, 2);
						self.scriptNode.onaudioprocess = function(ev) {
							var outputBuffer = ev.outputBuffer;

							var outL = outputBuffer.getChannelData(0);
							var outR = outputBuffer.getChannelData(1);

							var ended = self.render_samples();

							for (var sample = 0; sample < outputBuffer.length; sample++) {
								outL[sample] = Module.HEAP32[self.bufL/4 + sample] / 32768.0;
								outR[sample] = Module.HEAP32[self.bufR/4 + sample] / 32768.0;
							}

							if(ended) {
								self.scriptNode.disconnect(self.audioCtx.destination);
								self.playing = false;
								$rootScope.$emit('songEnded');
							}
						}

						self.init_player(self.bufL, self.bufR, self.bufSize, self.audioCtx.sampleRate);

						self.ready = true;

						for(var i in self.readyCallbacks) {
							self.readyCallbacks[i].call();
						}
					}
				};

				var scr = document.createElement('script');
				scr.setAttribute('type', 'text/javascript');
				scr.setAttribute('src', 'ay.js');
				document.body.appendChild(scr);
			}
		}
	};

	self.load = function(url, song, time, playing) {
		console.log('load', url, song, time, playing);
		if(self.playing)
			self.scriptNode.disconnect(self.audioCtx.destination);
		if(!song) song = 0;

		self.getPlayer(function() {
			console.info('loading AY file %c'+url, 'font-weight: bold');
			var mdxReq = new XMLHttpRequest();
			mdxReq.open("GET", 'ayfiles/'+url, true);
			mdxReq.responseType = "arraybuffer";

			mdxReq.onload = function(oEvent) {
				var mdx_data_len = mdxReq.response.byteLength;
				if(self.mdx_data) Module._free(self.mdx_data);
				self.mdx_data = Module._malloc(mdx_data_len);
				Module.HEAPU8.set(new Uint8Array(mdxReq.response), self.mdx_data);

				if(!self.play_file(self.mdx_data, mdx_data_len, song)) {
					var ns = self.get_num_songs();
					console.log('num songs', ns);
					console.log('song name', self.get_song_name(song));
					var html = '<ol>';
					for(var i = 0; i < ns; i++) {
						html += '<li><a href="javascript:" onclick="player.load(&quot;'+url+'&quot;, '+i+', 0, 1);">'+self.get_song_name(i)+'</a></li>';
					}
					html += '</ol>';
					document.getElementById('player').innerHTML = html;
					if(playing) {
						self.scriptNode.connect(self.audioCtx.destination);
						self.playing = true;
					}
				}
			};

			mdxReq.send();

			// self.audio.addEventListener('timeupdate',function () {
			// 	$rootScope.$emit('timeUpdate', self.audio.currentTime);
			// });
		});
	};
}

var player;
document.addEventListener("DOMContentLoaded", function(event) {
	player = new AYPlayer();
});
