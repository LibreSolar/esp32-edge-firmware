import Vue from 'vue';
import Vuetify from 'vuetify/lib';

Vue.use(Vuetify)

export default new Vuetify({
  theme: {
    dark: false,
    themes: {
      light: {
        primary: '#005e85',
        secondary: '#5c9aaf',
        accent: '#f5f5f5'
      },
      dark: {
        primary: '#005e85',
        secondary: '#5c9aaf',
        accent: '#616161'
      }
    }
  },
})
