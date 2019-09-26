/*
 *  This file is part of the DAC distribution (https://github.com/xainag/frost)
 *  Copyright (c) 2019 XAIN AG.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

package com.de.xain.emdac.api;

import com.de.xain.emdac.api.model.policy_server.PSClearPolicyListRequest;
import com.de.xain.emdac.api.model.policy_server.PSDelegatePolicyRequest;
import com.de.xain.emdac.api.model.policy_server.PSEmptyResponse;

import io.reactivex.Observable;
import retrofit2.Call;
import retrofit2.http.Body;
import retrofit2.http.PUT;

public interface PSService {

    @PUT("/policy")
    Call<PSEmptyResponse> clearPolicyList(@Body PSClearPolicyListRequest request);

    @PUT("/policy")
    Observable<PSEmptyResponse> delegatePolicy(@Body PSDelegatePolicyRequest request);
}
