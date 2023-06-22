/* eslint-disable import/order */
import 'normalize.css';

import './globalStyles.css';

import { env, initEnv } from '@ncoderz/env';
import { ConsoleLogSink, superlog, ConsoleFormatter, LogToken } from '@ncoderz/superlog';
import { Capacitor } from '@capacitor/core';
import { SuppressLongpressGesture } from 'capacitor-suppress-longpress-gesture';
// import { GameManager } from './game/GameManager';

console.log('Hello Browser World');

initEnv({
  app: 'jsw-web',
  version: '1.2.3',
});

// Init superlog
superlog.init({
  enabled: true,
  appName: 'jsw-web',
  metadata: {
    some: 'metadata',
  },
});

// Add console superlog sink
superlog.registerSink(
  new ConsoleLogSink(
    // new ObjectFormatter({
    //   argsOnly: false,
    // }),
    // new JsonFormatter({
    //   argsOnly: false,
    //   excludeSerializationErrors: true,
    // }),
    new ConsoleFormatter({
      // colourise: false,
      // padLevel: false,
      template: `${LogToken.level} [${LogToken.logger}]${LogToken.data}`,
      // superfast: true,
    }),
  ),
  {
    enabled: true,
    // supportedLevels: ['warn', 'fatal'],
    loggers: {
      default: 'debug',
      test: 'track',
    },
  },
);

const log = superlog.getLogger('entry');

log.info(env);
log.info(`Capacitor platform: ${Capacitor.getPlatform()}`);

function initApp(): void {
  // Suppress long presses in the WebView on iOS
  SuppressLongpressGesture.activateService();

  // const game = new GameManager();

  // game.init('mm');
  // // game.init('jsw');
  // // game.init('jsw2');
  // // game.init('wee');
  // // game.init('temoana');

  // (window as any).initGame = () => {
  //   return;
  // };
}

// eslint-disable-next-line @typescript-eslint/no-explicit-any
(window as any).initApp = initApp;
initApp();

const content = document.getElementById('content');

if (content) {
  // content.innerHTML = `<pre>${JSON.stringify(env, null, 2)}</pre>`;
}
