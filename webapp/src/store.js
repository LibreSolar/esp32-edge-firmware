import Vue from 'vue'
import Vuex from 'vuex'
import axios from 'axios'

Vue.use(Vuex)

export default new Vuex.Store({
  state: {
    chart_data: {},
    chart_data_keys: [],
    loading: true,
    devices: {},
    active_device: "",
    active_device_id: "",
    info: {}

  },
  mutations: {
    init_chart_data(state, new_data) {
      const keys = Object.keys(new_data)
      keys.forEach(key => {
        new_data[key] = Array(100)
        state.chart_data = new_data
      })
      state.chart_data_keys = keys
      console.log(state.chart_data)
    },
    update_chart_data(state, new_data) {
      state.chart_data_keys.forEach(key => {
        state.chart_data[key].shift()
        state.chart_data[key].push(new_data[key])
      });
      console.log(new_data);
      console.log(state.chart_data);
    }
  },
  actions: {
    init_chart_data( { commit }) {
      return axios.get("api/v1/ts/"+ this.state.active_device_id +"/output")
        .then(res => {
          commit("init_chart_data", res.data);
        })
        .catch(error => {
          console.log(error);
        })
    },
    update_chart_data({ commit }) {
      return axios.get("api/v1/ts/"+ this.state.active_device_id +"/output")
        .then(res => {
          commit("update_chart_data", res.data);
        })
        .catch(error => {
          console.log(error);
        });
    }
  }
})