// $Id: recfriio.cpp 5710 2008-09-19 15:04:04Z clworld $

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <signal.h>
#include <fcntl.h>

#include <inttypes.h>
#include <dirent.h>
#include <errno.h>

#include <math.h>

#include <err.h>

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <map>

#include <boost/scoped_ptr.hpp>

#include "setting.hpp"
#include "error.hpp"
#include "Recordable.hpp"
#ifdef B25
	#include "B25Decoder.hpp"
#endif /* defined(B25) */
#ifdef UDP
	#include "Udp.hpp"
#endif /* defined(UDP) */

#include "tssplitter_lite.hpp"

/**
 * usageの表示
 */
void usage(char *argv[])
{
	std::cerr << "usage: " << argv[0]
#ifdef B25
		<< " [--b25 [--round N] [--strip] [--EMM]]"
#endif /* defined(B25) */
#ifdef HDUS
		<< " [--hdus]"
		<< " [--hdp]"
#endif /* defined(HDUS) */
#ifdef UDP
		<< " [--udp ip [--port N]]"
#endif /* defined(UDP) */
		<< " [--lockfile lock] [--sid SID1,SID2]"
		<< " [--lnb]"
		<< " channel recsec destfile" << std::endl;
	std::cerr << "Channels:" << std::endl;
	std::cerr << "  13 - 62 : UHF13 - UHF62" << std::endl;
#ifdef HDUS
	std::cerr << "  K13 - K63 : CATV13 - CATV63" << std::endl;
#endif /* defined(HDUS) */
	std::cerr << "  B1 : BS朝日               C1 : 110CS #1" << std::endl;
	std::cerr << "  B2 : BS-TBS               C2 : 110CS #2" << std::endl;
	std::cerr << "  B3 : BSジャパン           C3 : 110CS #3" << std::endl;
	std::cerr << "  B4 : WOWOWプライム        C4 : 110CS #4" << std::endl;
	std::cerr << "  B5 : WOWOWライブ          C5 : 110CS #5" << std::endl;
	std::cerr << "  B6 : WOWOWシネマ          C6 : 110CS #6" << std::endl;
	std::cerr << "  B7 : スターチャンネル2/3  C7 : 110CS #7" << std::endl;
	std::cerr << "  B8 : BSアニマックス       C8 : 110CS #8" << std::endl;
	std::cerr << "  B9 : ディズニーチャンネル C9 : 110CS #9" << std::endl;
	std::cerr << "  B10: BS11                 C10: 110CS #a" << std::endl;
	std::cerr << "  B11: スターチャンネル1    C11: 110CS #b" << std::endl;
	std::cerr << "  B12: TwellV               C12: 110CS #c" << std::endl;
	std::cerr << "  B13: FOX bs238" << std::endl;
	std::cerr << "  B14: BSスカパー!" << std::endl;
	std::cerr << "  B15: 放送大学" << std::endl;
	std::cerr << "  B16: BS日テレ" << std::endl;
	std::cerr << "  B17: BSフジ" << std::endl;
	std::cerr << "  B18: NHK BS1" << std::endl;
	std::cerr << "  B19: NHK BSプレミアム" << std::endl;
	std::cerr << "  B20: 地デジ難視聴1(NHK/NHK-E/CX)" << std::endl;
	std::cerr << "  B21: 地デジ難視聴2(NTV/TBS/EX/TX)" << std::endl;
	std::cerr << "  B22: グリーンチャンネル" << std::endl;
	std::cerr << "  B23: J SPORTS 1" << std::endl;
	std::cerr << "  B24: J SPORTS 2" << std::endl;
	std::cerr << "  B25: IMAGICA BS" << std::endl;
	std::cerr << "  B26: J SPORTS 3" << std::endl;
	std::cerr << "  B27: J SPORTS 4" << std::endl;
	std::cerr << "  B28: BS釣りビジョン" << std::endl;
	std::cerr << "  B29: 日本映画専門チャンネル" << std::endl;
	std::cerr << "  B30: D-Life" << std::endl;
	exit(1);
}

/**
 * オプション情報
 */
struct Args {
#ifdef B25
	bool b25;
	int round;
	bool strip;
	bool emm;
#endif /* defined(B25) */
	char* lockfile;
	bool lnb;
	bool stdout;
	TunerType type;
	BandType band;
	int channel;
	bool forever;
	int recsec;
	char* destfile;
#ifdef UDP
	std::string ip;
	int port;
#endif /* defined(UDP) */
	bool splitter;
	char *sid_list;
};

/**
 * オプションの解析
 */
Args
parseOption(int argc, char *argv[])
{
	Args args = {
#ifdef B25
		false,
		4,
		false,
		false,
#endif /* defined(B25) */
		NULL,
		false,
		false,
		TUNER_FRIIO_WHITE,
		BAND_UHF,
		0,
		false,
		0,
		NULL,
#ifdef UDP
		"",
		UDP_PORT,
#endif /* defined(UDP) */
		false,
		NULL,
	};

#ifdef HDUS
	bool use_hdus = false;
	bool use_hdp = false;
#endif /* defined(HDUS) */

	while (1) {
		int option_index = 0;
		static option long_options[] = {
			{ "lockfile", 1, NULL, 'l' }, 
			{ "lnb", 0, NULL, 'n' }, 
#ifdef B25
			{ "b25",      0, NULL, 'b' },
			{ "B25",      0, NULL, 'b' },
			{ "round",    1, NULL, 'r' },
			{ "strip",    0, NULL, 's' },
			{ "EMM",      0, NULL, 'm' },
			{ "emm",      0, NULL, 'm' },
#endif /* defined(B25) */
#ifdef HDUS
			{ "hdus",      0, NULL, 'h' },
			{ "hdp",       0, NULL, 'd' },
#endif /* defined(HDUS) */
#ifdef UDP
			{ "udp",       1, NULL, 'u' },
			{ "port",      1, NULL, 'p' },
#endif /* defined(UDP) */
			{ "sid",      1, NULL, 'i'},
			{ 0,     0, NULL, 0   }
		};
		
		int r = getopt_long(argc, argv,
#ifdef B25
		                    "bsmr:"
#endif /* defined(B25) */
		                    "l:",
		                    long_options, &option_index);
		if (r < 0) {
			break;
		}
		
		switch (r) {
			case 'l':
				args.lockfile = optarg;
				break;
			case 'n':
				args.lnb = true;
				break;
#ifdef B25
			case 'b':
				args.b25 = true;
				break;
			case 'r':
				args.round = atoi(optarg);
				break;
			case 's':
				args.strip = true;
				break;
			case 'm':
				args.emm = true;
				break;
#endif /* defined(B25) */
#ifdef HDUS
			case 'h':
				use_hdus = true;
				break;
			case 'd':
				use_hdp = true;
				break;
#endif /* defined(HDUS) */
#ifdef UDP
			case 'u':
				args.ip = optarg;
				break;
			case 'p':
				args.port = atoi(optarg);
				break;
#endif /* defined(UDP) */
			case 'i':
				args.splitter = true;
				args.sid_list = optarg;
				break;
			default:
				break;
		}
	}
	
	if (argc - optind != 3) {
		usage(argv);
	}
	
	char* chstr    = argv[optind++];
	switch (chstr[0]) {
		case 'B':
		case 'b':
			args.type = TUNER_FRIIO_BLACK;
			args.band = BAND_BS;
			chstr++;
			break;
		case 'C':
		case 'c':
			args.type = TUNER_FRIIO_BLACK;
			args.band = BAND_CS;
			chstr++;
			break;
#ifdef HDUS
		case 'K':
		case 'k':
			if( use_hdus )
				args.type = TUNER_HDUS;
			else
				args.type = TUNER_HDP;
			args.band = BAND_CATV;
			chstr++;
			break;
#endif 
		default:
#ifdef HDUS
			if( use_hdus )
				args.type = TUNER_HDUS;
			else if( use_hdp )
				args.type = TUNER_HDP;
			else
				args.type = TUNER_FRIIO_WHITE;
#else
			args.type = TUNER_FRIIO_WHITE;
#endif /* defined(HDUS) */
			args.band = BAND_UHF;
	}
	args.channel   = atoi(chstr);
	char *recsecstr = argv[optind++];
	if (strcmp("-", recsecstr) == 0) {
		args.forever = true;
	}
	args.recsec    = atoi(recsecstr);
	args.destfile = argv[optind++];
	if (strcmp("-", args.destfile) == 0) {
		args.stdout = true;
	}
	
	return args;
}

/** シグナルを受けた */
static bool caughtSignal = false;

/** シグナルハンドラ */
void
sighandler(int arg)
{
	caughtSignal = true;
}

/** main */
int
main(int argc, char *argv[])
{
	Args args = parseOption(argc, argv);
	// 正常終了時戻り値
	int result = 0;
	
	// 引数確認
	switch (args.band) {
#ifdef HDUS
		case BAND_CATV:
			if (args.channel < 13 || 63 < args.channel) {
				std::cerr << "channel must be (13 <= channel <= 63)." << std::endl;
				exit(1);
			}
			break;
#endif /* defined(HDUS) */
		case BAND_UHF:
			if (args.channel < 13 || 62 < args.channel) {
				std::cerr << "channel must be (13 <= channel <= 62)." << std::endl;
				exit(1);
			}
			break;
		case BAND_BS:
			if (args.channel < 1 || 30 < args.channel) {
				std::cerr << "channel must be (B1 <= channel <= B30)." << std::endl;
				exit(1);
			}
			break;
		case BAND_CS:
			if (args.channel < 1 || 12 < args.channel) {
				std::cerr << "channel must be (C1 <= channel <= C12)." << std::endl;
				exit(1);
			}
			break;
		default:
			std::cerr << "unknown channel." << std::endl;
			exit(1);
	}
	
	if (!args.forever && args.recsec <= 0) {
		std::cerr << "recsec must be (recsec > 0)." << std::endl;
		exit(1);
	}
	
	// 録画時間の基準開始時間
	time_t time_start = time(NULL);
	
	// ログ出力先設定
	std::ostream& log = args.stdout ? std::cerr : std::cout;

#ifdef B25
	// B25初期化
	B25Decoder b25dec;
	if (args.b25) {
		try {
			b25dec.setRound(args.round);
			b25dec.setStrip(args.strip);
			b25dec.setEmmProcess(args.emm);
			b25dec.open();
			log << "B25Decoder initialized." << std::endl;
		} catch (b25_error& e) {
			std::cerr << e.what() << std::endl;
			
			// エラー時b25を行わず処理続行。終了ステータス1
			std::cerr << "disable b25 decoding." << std::endl;
			args.b25 = false;
			result = 1;
		}
	}
#endif /* defined(B25) */

#ifdef UDP
	// UDP初期化
	Udp udp;
	if( ! args.ip.empty() ){
		try{
			udp.setLog(&log);
			udp.init( args.ip, args.port );
		}
		catch( const char* e ){
			log << e << std::endl;
			log << "disable UDP." << std::endl;
		}
	}
#endif /* defined(UDP) */

	/* initialize splitter */
	splitbuf_t splitbuf;
	splitbuf.size = 0;
	splitter *splitter = NULL;
	int split_select_finish = TSS_ERROR;
	int code;
	if(args.splitter) {
		splitter = split_startup(args.sid_list);
		if(splitter->sid_list == NULL) {
			fprintf(stderr, "Cannot start TS splitter\n");
			return 1;
		}
	}

	// Tuner取得
	boost::scoped_ptr<Recordable> tuner(createRecordable(args.type));
#ifdef HDUS
	if( args.type == TUNER_HDUS ) log << "Tuner type is HDUS." << std::endl;
	else if( args.type == TUNER_HDP ) log << "Tuner type is HDP." << std::endl;
#endif /* defined(HDUS) */
	// ログ出力先設定
	tuner->setLog(&log);
	// ロックファイル設定
	if (args.lockfile != NULL) {
		tuner->setDetectLockFile(args.lockfile);
	}
	
	// Tuner初期化
	int retryCount = ERROR_RETRY_MAX;
	while (0 < retryCount) {
		try {
			// チューナopen
			bool r = tuner->open(args.lnb);
			if (!r) {
				std::cerr << "can't open tuner." << std::endl;
				exit(1);
			}
			
			// チャンネル設定
			tuner->setChannel(args.band, args.channel);
			
			// 開始時SignalLevel出力
			float lev_before = 0.0;
			int lev_retry_count = SIGNALLEVEL_RETRY_MAX;
			while (lev_before < SIGNALLEVEL_RETRY_THRESHOLD && 0 < lev_retry_count) {
				lev_before = tuner->getSignalLevel();
				log << "Signal level: " << lev_before << std::endl;
				
				lev_retry_count--;
				usleep(SIGNALLEVEL_RETRY_INTERVAL * 1000);
			}
		} catch (usb_error& e) {
			// リトライ処理
			retryCount--;
			std::cerr << e.what();
			if (retryCount <= 0) {
				std::cerr << " abort." << std::endl;
				exit(1);
			}
			std::cerr << " retry." << std::endl;
			
			tuner->close();
			usleep(ERROR_RETRY_INTERVAL * 1000);
			continue;
		}
		break;
	}
	
	// 出力先ファイルオープン
	FILE *dest = stdout;
	if (!args.stdout) {
		dest = fopen(args.destfile, "w");
		if (NULL == dest) {
			std::cerr << "can't open file '" << args.destfile << "' to write." << std::endl;
			exit(1);
		}
	}
	
	// 出力開始/時間計測
	log << "Output ts file." << std::endl;
	timeval tv_start;
	if (gettimeofday(&tv_start, NULL) < 0) {
		std::cerr << "gettimeofday failed." << std::endl;
		exit(1);
	}
	
	// SIGINT/SIGTERMキャッチ
	struct sigaction sa;
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = sighandler;
	sa.sa_flags = SA_RESTART;
	struct sigaction saDefault;
	memset(&saDefault, 0, sizeof(struct sigaction));
	saDefault.sa_handler = SIG_DFL;
	sigaction(SIGINT,  &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGPIPE, &sa, NULL);
	
	// 受信スレッド起動
	tuner->startStream();
	// データ読み出し
	uint32_t urb_error_cnt = 0;
	while (!caughtSignal && (args.forever || time(NULL) <= time_start + args.recsec)) {
		try {
			const uint8_t *buf = NULL;
			int rlen = tuner->getStream(&buf, 200);
			if (0 == rlen) {
				continue;
			}
			
#ifdef B25
			// B25を経由させる。
			if (args.b25) {
				static int f_b25_sync = 0;
				try {
					const uint8_t *b25buf = buf;
					b25dec.put(b25buf, rlen);
					rlen = b25dec.get(&b25buf);
					if (0 == rlen) {
						continue;
					}
					f_b25_sync = 1;
					buf = b25buf;
				} catch (b25_error& e) {
					if( f_b25_sync == 0 ){
						log << "Wait for B25 sync" << std::endl;
						continue;
					}
					log << "B25 Error: " << e.what() << std::endl;
					log << "Continue recording without B25." << std::endl;
					// b25停止、戻り値エラー
					args.b25 = false;
					result = 1;
				}
			}
#endif /* defined(B25) */

			if (args.splitter) {
				splitbuf.size = 0;
				while (rlen) {
					/* 分離対象PIDの抽出 */
					if (split_select_finish != TSS_SUCCESS) {
						split_select_finish = split_select(splitter, buf, rlen);
						if (split_select_finish == TSS_NULL) {
							/* mallocエラー発生 */
							log << "split_select malloc failed" << std::endl;
							args.splitter = false;
							result = 1;
							goto fin;
						} else if (split_select_finish != TSS_SUCCESS) {
							// 分離対象PIDが完全に抽出できるまで出力しない
							// 1秒程度余裕を見るといいかも
							time_t cur_time;
							time(&cur_time);
							if (cur_time - time_start > 4) {
								args.splitter = false;
								result = 1;
								goto fin;
							}
							break;
						}
					}
					/* 分離対象以外をふるい落とす */
					code = split_ts(splitter, buf, rlen, &splitbuf);
					if (code != TSS_SUCCESS) {
						log << "split_ts failed" << std::endl;
						break;
					}
					break;
				}

				rlen = splitbuf.size;
				buf = splitbuf.buffer;
			fin:
				;
			}

			fwrite(buf, 1, rlen, dest);

#ifdef UDP
			// UDP 配信
			udp.send(buf, rlen);
#endif /* defined(UDP) */

		} catch (usb_error& e) {
			if (urb_error_cnt <= URB_ERROR_MAX) {
				log << e.what() << std::endl;
				if (urb_error_cnt == URB_ERROR_MAX) {
					log << "Too many URB error." << std::endl;
				}
				urb_error_cnt++;
			}
		}
	}
	if (caughtSignal) {
		log << "interrupted." << std::endl;
	}
	// 受信スレッド停止
	tuner->stopStream();
	
	// シグナルハンドラを戻す。
	sigaction(SIGINT,  &saDefault, NULL);
	sigaction(SIGTERM, &saDefault, NULL);
	sigaction(SIGPIPE, &saDefault, NULL);

#ifdef B25
	// B25デコーダ内のデータを出力する。
	if (args.b25) {
		try {
			b25dec.flush();
			const uint8_t *buf = NULL;
			int rlen = b25dec.get(&buf);
			if (0 < rlen) {
				fwrite(buf, 1, rlen, dest);
			}
		} catch (b25_error& e) {
			log << "B25 Error: " << e.what() << std::endl;
			result = 1;
		}
	}
#endif /* defined(B25) */

	if (args.splitter) {
		split_shutdown(splitter);
	}

	// 時間計測
	timeval tv_end;
	if (gettimeofday(&tv_end, NULL) < 0) {
		err(1, "gettimeofday failed.");
	}
	
	// 出力先ファイルクローズ
	fflush(dest);
	if (!args.stdout) {
		fclose(dest);
	}
	log << "done." << std::endl;
	
#ifdef UDP
	// UDP クローズ
	udp.shutdown();
#endif /* defined(UDP) */
	
	// 録画時間出力
	timeval rec_time;
	timersub(&tv_end, &tv_start, &rec_time);
	log << "Rec time: " << rec_time.tv_sec << "." << std::setfill('0') << std::setw(6) << rec_time.tv_usec << " sec." << std::endl;
	
	// 終了時SignalLevel出力
	try {
		float lev_after = tuner->getSignalLevel();
		log << "Signal level: " << lev_after << std::endl;
	} catch (usb_error& e) {
		log << e.what() << " ignored." << std::endl;
	}
	
	return result;
}
