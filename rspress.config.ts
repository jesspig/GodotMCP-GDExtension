import { defineConfig } from '@rspress/core';
import mermaid from 'rspress-plugin-mermaid';

export default defineConfig({
  plugins: [mermaid()],
  root: 'docs',
  base: '/GodotMCP/',
  lang: 'zh',
  locales: [
    {
      lang: 'zh',
      label: '🇨🇳 简体中文',
      title: 'GodotMCP',
      description: '通过 Model Context Protocol 将 Godot 4.6+ 编辑器接入 AI 编码助手',
    },
    {
      lang: 'en',
      label: '🇺🇸 English',
      title: 'GodotMCP',
      description: 'Bridge between AI coding assistants and the Godot 4.6+ editor via the Model Context Protocol',
    },
  ],
  icon: '/favicon.svg',
  logoText: 'GodotMCP',
  outDir: 'doc_build',
  nav: [
    { text: 'guide', link: '/guide/' },
    { text: 'reference', link: '/reference/' },
    { text: 'changelog', link: '/changelog/' },
  ],
  themeConfig: {
    sidebar: {
      '/': [
        {
          text: 'guide',
          collapsible: true,
          items: [
            { text: 'gettingStarted', link: '/guide/getting-started' },
            { text: 'architecture', link: '/guide/architecture' },
            { text: 'building', link: '/guide/building' },
          ],
        },
        {
          text: 'reference',
          collapsible: true,
          items: [
            { text: 'toolsCatalog', link: '/reference/tools-catalog' },
            { text: 'clientConfig', link: '/reference/client-config' },
            { text: 'protocol', link: '/reference/protocol' },
            { text: 'faq', link: '/reference/faq' },
            { text: 'clientQuirks', link: '/reference/client-quirks' },
            { text: 'lspClient', link: '/reference/lsp-client' },
            { text: 'projectSettingsExt', link: '/reference/project-settings-ext' },
            { text: 'csharpSolution', link: '/reference/csharp-solution' },
          ],
        },
        {
          text: 'changelog',
          collapsible: true,
          items: [
            { text: 'overview', link: '/changelog/' },
            { text: 'v0.2.0', link: '/changelog/v0-2-0' },
            { text: 'v0.1.5', link: '/changelog/v0-1-5' },
            { text: 'v0.1.4', link: '/changelog/v0-1-4' },
            { text: 'v0.1.3', link: '/changelog/v0-1-3' },
            { text: 'v0.1.2', link: '/changelog/v0-1-2' },
            { text: 'v0.1.1', link: '/changelog/v0-1-1' },
            { text: 'v0.1.0', link: '/changelog/v0-1-0' },
          ],
        },
      ],
    },
  },
});
