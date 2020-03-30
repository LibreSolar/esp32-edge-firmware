module.exports = {
  productionSourceMap: false,
  devServer: {
    proxy: {
      '/api': {
        target: 'http://localhost:3000',
        changeOrigin: true,
        ws: true
      }
    }
  },
  chainWebpack: (config) => {
    config.optimization.delete('splitChunks'),
    config.module
      .rule('images')
      .use('url-loader')
      .tap(options => Object.assign({}, options, { name: '[name].[ext]' }));
  },
  css: {
    extract: {
      filename: '[name].css',
      chunkFilename: '[name].css',
    },
  },
  configureWebpack: {
    output: {
      filename: '[name].js',
      chunkFilename: '[name].js',
    }
  }
}
