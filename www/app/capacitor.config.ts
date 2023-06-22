import { CapacitorConfig } from '@capacitor/cli';

const config: CapacitorConfig = {
  appId: 'jsw.mm',
  appName: 'Manic Miner 40',
  webDir: 'dist',
  bundledWebRuntime: false,
  android: {
    initialFocus: true,
    // captureInput: true,
  },
  ios: {
    contentInset: 'never',
    scrollEnabled: false,
  },
};

// eslint-disable-next-line import/no-default-export
export default config;
