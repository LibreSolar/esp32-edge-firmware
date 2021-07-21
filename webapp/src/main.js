import Vue from 'vue'
import App from './App.vue'
import router from './router'
import store from './store'
import axios from 'axios'
import vuetify from './plugins/vuetify'

Vue.config.productionTip = false
Vue.prototype.$ajax = axios

new Vue({
  vuetify,
  router,
  store,
  render: h => h(App)
}).$mount('#app');

router.replace('/')
