import basicSsl from '@vitejs/plugin-basic-ssl';
import { defineConfig } from 'vite';
import tsconfigPaths from 'vite-tsconfig-paths';

// eslint-disable-next-line import/no-default-export
export default defineConfig({
  plugins: [tsconfigPaths(), basicSsl()],
  build: {
    rollupOptions: {
      //
    },
    chunkSizeWarningLimit: 2048,
  },
});
