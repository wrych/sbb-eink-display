import { defineConfig } from 'vite'

// https://vitejs.dev/config/
export default defineConfig({
    // Set base to './' for relative paths in the build output
    // This ensures the app works correctly when hosted in a sub-folder on GitHub Pages
    base: './',
})
