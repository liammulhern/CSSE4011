import path from 'path';
import { defineConfig, loadEnv } from 'vite'
import vue from '@vitejs/plugin-vue'
import tailwind from 'tailwindcss'
import autoprefixer from 'autoprefixer'

export default defineConfig(({ mode }) => {
  const rootDir = path.resolve(__dirname, 'src');
  const envDir = path.resolve(__dirname, '..');
  const env = loadEnv(mode, envDir, '');
  const production = env.NODE_ENV === 'production';

  return {
    root: rootDir,
    base: '/',
    appType: 'spa',
    plugins: [
      vue(),
    ],
    css: {
      postcss: {
        plugins: [
          autoprefixer(),
          tailwind(),
        ],
      },
    },
    resolve: {
      alias: {
        '@': rootDir,
      }
    },
    build: {
      minify: production,
      sourcemap: production,
      outDir: path.resolve(rootDir, '..', 'dist'),
    },
    preview: {
      port: 5173,
      strictPort: true,
     },
     server: {
      port: 5173,
      strictPort: true,
      host: true,
      origin: "http://0.0.0.0:5173",
     },
  }
});
