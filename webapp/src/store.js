import Vue from 'vue'
import Vuex from 'vuex'
import axios from 'axios'

Vue.use(Vuex)

export default new Vuex.Store({
  state: {
    chartData: {},
    chartDataKeys: [],
    loading: true,
    devices: {},
    activeDevice: "",
    activeDeviceId: "",
    info: null

  },
  mutations: {
    initChartData(state, newData) {
      const keys = Object.keys(newData)
      keys.forEach(key => {
        newData[key] = Array(100)
        state.chartData = newData
      })
      state.chartDataKeys = keys
    },
    updateChartData(state, newData) {
      state.chartDataKeys.forEach(key => {
        state.chartData[key].shift()
        state.chartData[key].push(newData[key])
      });
    }
  },
  actions: {
    initChartData( { commit }) {
      return axios.get("api/v1/ts/"+ this.state.activeDeviceId +"/output")
        .then(res => {
          commit("initChartData", res.data);
        })
        .catch(error => {
          console.log(error);
        })
    },
    updateChartData({ commit }) {
      return axios.get("api/v1/ts/"+ this.state.activeDeviceId +"/output")
        .then(res => {
          commit("updateChartData", res.data);
        })
        .catch(error => {
          console.log(error);
        });
    }
  }
})