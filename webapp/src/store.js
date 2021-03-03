import Vue from 'vue'
import Vuex from 'vuex'
import axios from 'axios'
import info from "../info.json"

Vue.use(Vuex)

export default new Vuex.Store({
  state: {
    chartData: {},
    chartDataKeys: [],
    loading: true,
    devices: {},
    activeDevice: "No Devices connected..",
    activeDeviceId: "",
    info: info
  },
  mutations: {
    changeDevice(state, key) {
      state.activeDevice = key
      state.activeDeviceId = state.devices[key]
    },
    saveDevices(state, devices) {
      state.devices = devices
      state.activeDevice = Object.keys(devices)[0]
      state.activeDeviceId = Object.values(devices)[0]
      state.loading = false
    },
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
    getDevices( { commit }) {
      return  axios.get('api/v1/ts/')
        .then(res => {
          if (res.data) {
            commit('saveDevices', res.data)
        }
      })
    },
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